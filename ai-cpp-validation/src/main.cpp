#include <iostream>
#include <string>

#include "validation.hpp"

namespace {

void PrintUsage(const char* program_name) {
    std::cerr
        << "Usage: " << program_name << " [OPTIONS]\n\n"
        << "Options:\n"
        << "  --model <path>       Encrypted model file (IV || ciphertext)\n"
        << "  --signature <path>   Detached ECDSA signature (.sig)\n"
        << "  --engine <name>      Cryptographic engine: openssl | pkcs11\n"
        << "  --aes-key <path>     [openssl] Hex-encoded AES-256 key file\n"
        << "  --public-key <path>  [openssl] ECDSA public key (PEM)\n"
        << "  --pkcs11-module <p>  [pkcs11] Module path (default: "
           "/usr/lib/libsmw_pkcs11.so.5)\n"
        << "  --pkcs11-slot <N>    [pkcs11] Token slot ID (default: 0)\n"
        << "  --pkcs11-pin <pin>   [pkcs11] Optional login PIN (SMW needs none)\n"
        << "  --aes-key-label <s>  [pkcs11] AES object label\n"
        << "  --aes-key-id <hex>   [pkcs11] AES object CKA_ID\n"
        << "  --verify-key-label <s> [pkcs11] EC public-key object label\n"
        << "  --verify-key-id <hex>  [pkcs11] EC public-key object CKA_ID\n"
        << "  --out <path>         Optional: write decrypted model if verified\n"
        << "  --chunk-size <N>     Optional: streaming chunk size in bytes\n";
}

bool NextValue(int argc, char** argv, int& i, const std::string& flag,
               std::string& value) {
    if (i + 1 >= argc) {
        std::cerr << "Missing value for " << flag << "\n";
        return false;
    }
    value = argv[++i];
    return true;
}

bool ParseUnsignedLong(const std::string& value, unsigned long& parsed) {
    if (value.empty() || value.front() == '+' || value.front() == '-') {
        return false;
    }
    try {
        size_t consumed = 0;
        int base = value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0
                       ? 16
                       : 10;
        parsed = std::stoul(value, &consumed, base);
        return consumed == value.size();
    } catch (...) {
        return false;
    }
}

} // namespace

int main(int argc, char** argv) {
    imx93::ValidationConfig config;
    std::string engine_arg = "openssl";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string value;
        if (arg == "--model" && NextValue(argc, argv, i, arg, value)) {
            config.model_path = value;
        } else if (arg == "--signature" && NextValue(argc, argv, i, arg, value)) {
            config.signature_path = value;
        } else if (arg == "--engine" && NextValue(argc, argv, i, arg, value)) {
            engine_arg = value;
        } else if (arg == "--aes-key" && NextValue(argc, argv, i, arg, value)) {
            config.aes_key_path = value;
        } else if (arg == "--public-key" &&
                   NextValue(argc, argv, i, arg, value)) {
            config.public_key_path = value;
        } else if (arg == "--pkcs11-module" &&
                   NextValue(argc, argv, i, arg, value)) {
            config.pkcs11_module = value;
        } else if (arg == "--pkcs11-slot" &&
                   NextValue(argc, argv, i, arg, value)) {
            if (!ParseUnsignedLong(value, config.pkcs11_slot)) {
                std::cerr << "Invalid PKCS#11 slot: " << value << "\n";
                return 1;
            }
        } else if (arg == "--pkcs11-pin" &&
                   NextValue(argc, argv, i, arg, value)) {
            config.pkcs11_pin = value;
        } else if (arg == "--aes-key-label" &&
                   NextValue(argc, argv, i, arg, value)) {
            config.aes_key_label = value;
        } else if (arg == "--aes-key-id" &&
                   NextValue(argc, argv, i, arg, value)) {
            if (!imx93::ParseHex(value, config.aes_key_id)) {
                std::cerr << "Invalid hex AES key ID: " << value << "\n";
                return 1;
            }
        } else if (arg == "--verify-key-label" &&
                   NextValue(argc, argv, i, arg, value)) {
            config.verify_key_label = value;
        } else if (arg == "--verify-key-id" &&
                   NextValue(argc, argv, i, arg, value)) {
            if (!imx93::ParseHex(value, config.verify_key_id)) {
                std::cerr << "Invalid hex verification key ID: " << value
                          << "\n";
                return 1;
            }
        } else if (arg == "--out" && NextValue(argc, argv, i, arg, value)) {
            config.output_path = value;
        } else if (arg == "--chunk-size" &&
                   NextValue(argc, argv, i, arg, value)) {
            unsigned long chunk_size = 0;
            if (!ParseUnsignedLong(value, chunk_size) || chunk_size == 0) {
                std::cerr << "Invalid chunk size: " << value << "\n";
                return 1;
            }
            config.chunk_size = static_cast<size_t>(chunk_size);
        } else if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown or malformed argument: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }

    if (engine_arg == "openssl") {
        config.engine = imx93::Engine::OpenSSL;
    } else if (engine_arg == "pkcs11" || engine_arg == "ele") {
        config.engine = imx93::Engine::Pkcs11;
        engine_arg = "pkcs11";
    } else {
        std::cerr << "Invalid --engine '" << engine_arg
                  << "' (expected openssl or pkcs11)\n";
        return 1;
    }

    if (config.model_path.empty() || config.signature_path.empty()) {
        std::cerr << "--model and --signature are required\n";
        PrintUsage(argv[0]);
        return 1;
    }
    if (config.engine == imx93::Engine::OpenSSL &&
        (config.aes_key_path.empty() || config.public_key_path.empty())) {
        std::cerr << "openssl engine requires --aes-key and --public-key\n";
        PrintUsage(argv[0]);
        return 1;
    }
    if (config.engine == imx93::Engine::Pkcs11 &&
        config.aes_key_label.empty() && config.aes_key_id.empty()) {
        std::cerr << "pkcs11 engine requires --aes-key-label or --aes-key-id\n";
        PrintUsage(argv[0]);
        return 1;
    }
    if (config.engine == imx93::Engine::Pkcs11 &&
        config.verify_key_label.empty() && config.verify_key_id.empty()) {
        std::cerr << "pkcs11 engine requires --verify-key-label or "
                     "--verify-key-id\n";
        PrintUsage(argv[0]);
        return 1;
    }

    std::cout << "Validating " << config.model_path << " using '" << engine_arg
              << "' engine...\n";

    imx93::ValidationRunner runner;
    imx93::ValidationResult result = runner.Run(config);

    imx93::CliReporter reporter;
    reporter.Print(result);

    return result.integrity_pass ? 0 : 1;
}
