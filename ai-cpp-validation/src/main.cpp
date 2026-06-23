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
        << "  --engine <name>      Cryptographic engine: openssl | ele\n"
        << "  --aes-key <path>     [openssl] Hex-encoded AES-256 key file\n"
        << "  --public-key <path>  [openssl] ECDSA public key (PEM)\n"
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
        } else if (arg == "--out" && NextValue(argc, argv, i, arg, value)) {
            config.output_path = value;
        } else if (arg == "--chunk-size" &&
                   NextValue(argc, argv, i, arg, value)) {
            config.chunk_size = static_cast<size_t>(std::stoul(value));
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
    } else if (engine_arg == "ele") {
        config.engine = imx93::Engine::ELE;
    } else {
        std::cerr << "Invalid --engine '" << engine_arg
                  << "' (expected openssl or ele)\n";
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

    std::cout << "Validating " << config.model_path << " using '" << engine_arg
              << "' engine...\n";

    imx93::ValidationRunner runner;
    imx93::ValidationResult result = runner.Run(config);

    imx93::CliReporter reporter;
    reporter.Print(result);

    return result.integrity_pass ? 0 : 1;
}
