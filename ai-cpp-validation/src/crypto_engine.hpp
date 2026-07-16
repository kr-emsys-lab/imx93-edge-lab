#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace imx93 {

// Cryptographic backend abstraction. Concrete engines provide either a
// software path (OpenSSL/libcrypto) or a PKCS#11 hardware path. Decryption and
// signature verification are streaming so the model can be processed in
// chunks.
class CryptoEngine {
public:
    virtual ~CryptoEngine() = default;

    virtual std::string name() const = 0;
    virtual std::string last_error() const { return {}; }

    // AES-256-CBC decryption, fed one ciphertext chunk at a time. Engine key
    // configuration is supplied when the concrete engine is constructed.
    virtual bool DecryptBegin(const std::vector<uint8_t>& iv) = 0;
    virtual bool DecryptUpdate(const uint8_t* in, size_t in_len,
                               std::vector<uint8_t>& out) = 0;
    virtual bool DecryptFinish(std::vector<uint8_t>& out) = 0;

    // ECDSA P-256 / SHA-256 verification over the decrypted plaintext,
    // fed one plaintext chunk at a time, then checked against the detached
    // signature.
    virtual bool VerifyBegin() = 0;
    virtual bool VerifyUpdate(const uint8_t* data, size_t len) = 0;
    virtual bool VerifyFinal(const std::vector<uint8_t>& signature) = 0;

    // IV length expected at the front of the encrypted model file.
    static constexpr size_t kIvLength = 16;
    // AES-256 key length in bytes.
    static constexpr size_t kKeyLength = 32;
};

} // namespace imx93
