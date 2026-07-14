#include "validation.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>

#include "openssl_engine.hpp"
#ifdef HAS_SMW_PKCS11
#include "pkcs11_engine.hpp"
#endif

namespace imx93 {

namespace {

using Clock = std::chrono::high_resolution_clock;

double ElapsedMs(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::unique_ptr<CryptoEngine> MakeEngine(const ValidationConfig& config,
                                         std::vector<uint8_t> aes_key,
                                         std::string& error) {
    switch (config.engine) {
        case Engine::OpenSSL:
            return std::make_unique<OpenSslEngine>(std::move(aes_key),
                                                    config.public_key_path);
        case Engine::Pkcs11:
#ifdef HAS_SMW_PKCS11
        {
            Pkcs11Config pkcs11_config;
            pkcs11_config.module_path = config.pkcs11_module;
            pkcs11_config.slot = static_cast<CK_SLOT_ID>(config.pkcs11_slot);
            pkcs11_config.pin = config.pkcs11_pin;
            pkcs11_config.aes_key = {config.aes_key_label,
                                     config.aes_key_id};
            pkcs11_config.verify_key = {config.verify_key_label,
                                        config.verify_key_id};
            auto engine = std::make_unique<Pkcs11Engine>(
                std::move(pkcs11_config));
            if (!engine->ready()) {
                error = engine->last_error();
                return nullptr;
            }
            return engine;
        }
#else
            error = "PKCS#11 engine not compiled; rebuild with "
                    "-DENABLE_SMW_PKCS11=ON";
            return nullptr;
#endif
    }
    error = "Unknown engine";
    return nullptr;
}

} // namespace

bool ReadFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f),
               std::istreambuf_iterator<char>());
    return true;
}

bool WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(f);
}

bool ParseHex(const std::string& value, std::vector<uint8_t>& out) {
    std::string hex = value;
    hex.erase(std::remove_if(hex.begin(), hex.end(),
                             [](unsigned char c) { return std::isspace(c); }),
              hex.end());
    if (hex.empty() || hex.size() % 2 != 0 ||
        !std::all_of(hex.begin(), hex.end(), [](unsigned char c) {
            return std::isxdigit(c);
        })) {
        return false;
    }
    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        out.push_back(static_cast<uint8_t>(
            std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return true;
}

bool ReadHexKey(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path);
    if (!f) return false;
    std::string hex((std::istreambuf_iterator<char>(f)),
                    std::istreambuf_iterator<char>());
    return ParseHex(hex, out);
}

bool ModelDecryptor::Decrypt(CryptoEngine& engine,
                             const std::vector<uint8_t>& iv,
                             const std::vector<uint8_t>& ciphertext,
                             size_t chunk_size,
                             std::vector<uint8_t>& plaintext_out) {
    plaintext_out.clear();
    if (!engine.DecryptBegin(iv)) return false;

    std::vector<uint8_t> chunk_out;
    for (size_t offset = 0; offset < ciphertext.size(); offset += chunk_size) {
        size_t len = std::min(chunk_size, ciphertext.size() - offset);
        if (!engine.DecryptUpdate(ciphertext.data() + offset, len, chunk_out)) {
            return false;
        }
        plaintext_out.insert(plaintext_out.end(), chunk_out.begin(),
                             chunk_out.end());
    }
    if (!engine.DecryptFinish(chunk_out)) return false;
    plaintext_out.insert(plaintext_out.end(), chunk_out.begin(),
                         chunk_out.end());
    return true;
}

bool SignatureVerifier::Verify(CryptoEngine& engine,
                               const std::vector<uint8_t>& plaintext,
                               const std::vector<uint8_t>& signature,
                               size_t chunk_size) {
    if (!engine.VerifyBegin()) return false;
    for (size_t offset = 0; offset < plaintext.size(); offset += chunk_size) {
        size_t len = std::min(chunk_size, plaintext.size() - offset);
        if (!engine.VerifyUpdate(plaintext.data() + offset, len)) return false;
    }
    return engine.VerifyFinal(signature);
}

ValidationResult ValidationRunner::Run(const ValidationConfig& config) {
    ValidationResult result;
    result.engine =
        (config.engine == Engine::Pkcs11) ? "pkcs11" : "openssl";

    if (config.chunk_size == 0) {
        result.error = "Chunk size must be greater than zero";
        return result;
    }

    std::vector<uint8_t> encrypted;
    if (!ReadFile(config.model_path, encrypted)) {
        result.error = "Failed to read model: " + config.model_path;
        return result;
    }
    if (encrypted.size() <= CryptoEngine::kIvLength) {
        result.error = "Encrypted model too small (missing IV or ciphertext)";
        return result;
    }

    std::vector<uint8_t> key;
    if (config.engine == Engine::OpenSSL &&
        (!ReadHexKey(config.aes_key_path, key) ||
         key.size() != CryptoEngine::kKeyLength)) {
        result.error = "Failed to read 32-byte hex AES key: " +
                       config.aes_key_path;
        return result;
    }

    std::vector<uint8_t> signature;
    if (!ReadFile(config.signature_path, signature) || signature.empty()) {
        result.error = "Failed to read signature: " + config.signature_path;
        return result;
    }

    std::unique_ptr<CryptoEngine> engine =
        MakeEngine(config, std::move(key), result.error);
    if (!engine) return result;
    result.engine = engine->name();

    std::vector<uint8_t> iv(encrypted.begin(),
                            encrypted.begin() + CryptoEngine::kIvLength);
    std::vector<uint8_t> ciphertext(
        encrypted.begin() + CryptoEngine::kIvLength, encrypted.end());

    auto total_start = Clock::now();

    ModelDecryptor decryptor;
    std::vector<uint8_t> plaintext;
    auto dec_start = Clock::now();
    bool decrypted = decryptor.Decrypt(*engine, iv, ciphertext,
                                       config.chunk_size, plaintext);
    auto dec_end = Clock::now();
    result.decrypt_latency_ms = ElapsedMs(dec_start, dec_end);
    if (!decrypted) {
        result.error = engine->last_error();
        if (result.error.empty()) {
            result.error =
                "Decryption failed (wrong key or corrupted ciphertext)";
        }
        result.total_latency_ms = ElapsedMs(total_start, Clock::now());
        return result;
    }

    SignatureVerifier verifier;
    auto ver_start = Clock::now();
    result.integrity_pass = verifier.Verify(*engine, plaintext, signature,
                                            config.chunk_size);
    auto ver_end = Clock::now();
    result.verify_latency_ms = ElapsedMs(ver_start, ver_end);

    result.total_latency_ms = ElapsedMs(total_start, Clock::now());

    if (!result.integrity_pass) {
        result.error = engine->last_error();
        if (result.error.empty()) {
            result.error = "Signature verification failed";
        }
    } else if (!config.output_path.empty()) {
        if (!WriteFile(config.output_path, plaintext)) {
            result.error = "Verified, but failed to write output: " +
                           config.output_path;
        }
    }

    return result;
}

void CliReporter::Print(const ValidationResult& result) const {
    const std::string status = result.integrity_pass ? "PASS" : "FAIL";

    std::cout << "\n+" << std::string(12, '-') << "+" << std::string(8, '-')
              << "+" << std::string(16, '-') << "+" << std::string(16, '-')
              << "+" << std::string(15, '-') << "+\n";
    std::cout << "| " << std::left << std::setw(10) << "Engine"
              << " | " << std::setw(6) << "Result"
              << " | " << std::setw(14) << "Decrypt (ms)"
              << " | " << std::setw(14) << "Verify (ms)"
              << " | " << std::setw(13) << "Total (ms)" << " |\n";
    std::cout << "+" << std::string(12, '-') << "+" << std::string(8, '-')
              << "+" << std::string(16, '-') << "+" << std::string(16, '-')
              << "+" << std::string(15, '-') << "+\n";
    std::cout << "| " << std::left << std::setw(10) << result.engine
              << " | " << std::setw(6) << status
              << " | " << std::setw(14) << std::fixed << std::setprecision(3)
              << result.decrypt_latency_ms
              << " | " << std::setw(14) << std::fixed << std::setprecision(3)
              << result.verify_latency_ms
              << " | " << std::setw(13) << std::fixed << std::setprecision(3)
              << result.total_latency_ms << " |\n";
    std::cout << "+" << std::string(12, '-') << "+" << std::string(8, '-')
              << "+" << std::string(16, '-') << "+" << std::string(16, '-')
              << "+" << std::string(15, '-') << "+\n";

    if (!result.error.empty()) {
        std::cout << "Note: " << result.error << "\n";
    }
}

} // namespace imx93
