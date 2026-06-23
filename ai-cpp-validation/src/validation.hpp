#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "crypto_engine.hpp"

namespace imx93 {

enum class Engine {
    OpenSSL,
    ELE,
};

struct ValidationConfig {
    std::string model_path;       // encrypted model (IV || ciphertext)
    std::string signature_path;   // detached ECDSA signature (.sig)
    Engine engine = Engine::OpenSSL;
    std::string aes_key_path;     // software mode: hex-encoded AES-256 key file
    std::string public_key_path;  // software mode: ECDSA public key (PEM)
    std::string output_path;      // optional: write decrypted plaintext here
    size_t chunk_size = 64 * 1024;
};

struct ValidationResult {
    std::string engine;
    bool integrity_pass = false;
    double decrypt_latency_ms = 0.0;
    double verify_latency_ms = 0.0;
    double total_latency_ms = 0.0;
    std::string error;
};

// Streams the encrypted model through a CryptoEngine, accumulating plaintext.
class ModelDecryptor {
public:
    bool Decrypt(CryptoEngine& engine, const std::vector<uint8_t>& key,
                 const std::vector<uint8_t>& iv,
                 const std::vector<uint8_t>& ciphertext, size_t chunk_size,
                 std::vector<uint8_t>& plaintext_out);
};

// Verifies the detached signature over the decrypted plaintext.
class SignatureVerifier {
public:
    bool Verify(CryptoEngine& engine, const std::vector<uint8_t>& plaintext,
                const std::vector<uint8_t>& signature, size_t chunk_size);
};

class ValidationRunner {
public:
    ValidationResult Run(const ValidationConfig& config);
};

class CliReporter {
public:
    void Print(const ValidationResult& result) const;
};

// File / encoding helpers shared by the runtime.
bool ReadFile(const std::string& path, std::vector<uint8_t>& out);
bool WriteFile(const std::string& path, const std::vector<uint8_t>& data);
bool ReadHexKey(const std::string& path, std::vector<uint8_t>& out);

} // namespace imx93
