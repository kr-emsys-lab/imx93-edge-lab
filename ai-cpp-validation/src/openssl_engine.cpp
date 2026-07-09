#include "openssl_engine.hpp"

#include <cstdio>
#include <iostream>

#include <openssl/err.h>
#include <openssl/pem.h>

namespace imx93 {

namespace {
void PrintOpenSslError(const char* context) {
    unsigned long err = ERR_get_error();
    char buf[256];
    ERR_error_string_n(err, buf, sizeof(buf));
    std::cerr << "OpenSSL error (" << context << "): " << buf << "\n";
}
} // namespace

OpenSslEngine::OpenSslEngine(const std::string& public_key_path)
    : public_key_path_(public_key_path) {
    FILE* fp = std::fopen(public_key_path_.c_str(), "r");
    if (fp == nullptr) {
        std::cerr << "Failed to open public key: " << public_key_path_ << "\n";
        return;
    }
    public_key_ = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
    std::fclose(fp);
    if (public_key_ == nullptr) {
        PrintOpenSslError("PEM_read_PUBKEY");
    }
}

OpenSslEngine::~OpenSslEngine() {
    if (cipher_ctx_ != nullptr) EVP_CIPHER_CTX_free(cipher_ctx_);
    if (md_ctx_ != nullptr) EVP_MD_CTX_free(md_ctx_);
    if (public_key_ != nullptr) EVP_PKEY_free(public_key_);
}

bool OpenSslEngine::DecryptBegin(const std::vector<uint8_t>& key,
                                 const std::vector<uint8_t>& iv) {
    if (key.size() != kKeyLength || iv.size() != kIvLength) {
        std::cerr << "Invalid AES key (" << key.size() << "B) or IV ("
                  << iv.size() << "B)\n";
        return false;
    }
    cipher_ctx_ = EVP_CIPHER_CTX_new();
    if (cipher_ctx_ == nullptr) {
        PrintOpenSslError("EVP_CIPHER_CTX_new");
        return false;
    }
    if (EVP_DecryptInit_ex(cipher_ctx_, EVP_aes_256_cbc(), nullptr, key.data(),
                           iv.data()) != 1) {
        PrintOpenSslError("EVP_DecryptInit_ex");
        return false;
    }
    return true;
}

bool OpenSslEngine::DecryptUpdate(const uint8_t* in, size_t in_len,
                                  std::vector<uint8_t>& out) {
    if (cipher_ctx_ == nullptr) return false;
    // Worst-case output size for a CBC block cipher update.
    out.resize(in_len + EVP_MAX_BLOCK_LENGTH);
    int out_len = 0;
    if (EVP_DecryptUpdate(cipher_ctx_, out.data(), &out_len, in,
                          static_cast<int>(in_len)) != 1) {
        PrintOpenSslError("EVP_DecryptUpdate");
        return false;
    }
    out.resize(static_cast<size_t>(out_len));
    return true;
}

bool OpenSslEngine::DecryptFinish(std::vector<uint8_t>& out) {
    if (cipher_ctx_ == nullptr) return false;
    out.resize(EVP_MAX_BLOCK_LENGTH);
    int out_len = 0;
    if (EVP_DecryptFinal_ex(cipher_ctx_, out.data(), &out_len) != 1) {
        // A wrong key or corrupted ciphertext fails PKCS#7 padding here.
        PrintOpenSslError("EVP_DecryptFinal_ex");
        return false;
    }
    out.resize(static_cast<size_t>(out_len));
    return true;
}

bool OpenSslEngine::VerifyBegin() {
    if (public_key_ == nullptr) return false;
    md_ctx_ = EVP_MD_CTX_new();
    if (md_ctx_ == nullptr) {
        PrintOpenSslError("EVP_MD_CTX_new");
        return false;
    }
    if (EVP_DigestVerifyInit(md_ctx_, nullptr, EVP_sha256(), nullptr,
                             public_key_) != 1) {
        PrintOpenSslError("EVP_DigestVerifyInit");
        return false;
    }
    return true;
}

bool OpenSslEngine::VerifyUpdate(const uint8_t* data, size_t len) {
    if (md_ctx_ == nullptr) return false;
    if (EVP_DigestVerifyUpdate(md_ctx_, data, len) != 1) {
        PrintOpenSslError("EVP_DigestVerifyUpdate");
        return false;
    }
    return true;
}

bool OpenSslEngine::VerifyFinal(const std::vector<uint8_t>& signature) {
    if (md_ctx_ == nullptr) return false;
    // EVP_DigestVerifyFinal returns 1 for a valid signature, 0 for a mismatch.
    int rc = EVP_DigestVerifyFinal(md_ctx_, signature.data(), signature.size());
    if (rc == 1) return true;
    if (rc != 0) PrintOpenSslError("EVP_DigestVerifyFinal");
    return false;
}

} // namespace imx93
