#pragma once

#include <string>

#include <openssl/evp.h>

#include "crypto_engine.hpp"

namespace imx93 {

// Software CryptoEngine backed by OpenSSL libcrypto. Keys live in user-space
// memory, so this path is intended for local development and logic validation
// only (see README.md). The hardware-isolated path is provided by EleEngine.
class OpenSslEngine : public CryptoEngine {
public:
    explicit OpenSslEngine(const std::string& public_key_path);
    ~OpenSslEngine() override;

    std::string name() const override { return "openssl"; }

    bool DecryptBegin(const std::vector<uint8_t>& key,
                      const std::vector<uint8_t>& iv) override;
    bool DecryptUpdate(const uint8_t* in, size_t in_len,
                       std::vector<uint8_t>& out) override;
    bool DecryptFinish(std::vector<uint8_t>& out) override;

    bool VerifyBegin() override;
    bool VerifyUpdate(const uint8_t* data, size_t len) override;
    bool VerifyFinal(const std::vector<uint8_t>& signature) override;

private:
    std::string public_key_path_;
    EVP_PKEY* public_key_ = nullptr;
    EVP_CIPHER_CTX* cipher_ctx_ = nullptr;
    EVP_MD_CTX* md_ctx_ = nullptr;
};

} // namespace imx93
