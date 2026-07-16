#pragma once

#include <string>
#include <vector>

#include "crypto_engine.hpp"

#define CK_PTR *
#define CK_DECLARE_FUNCTION(returnType, name) returnType name
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) returnType(*name)
#define CK_CALLBACK_FUNCTION(returnType, name) returnType(*name)
#ifndef NULL_PTR
#define NULL_PTR nullptr
#endif
#include <pkcs11.h>

namespace imx93 {

struct Pkcs11KeySelector {
    std::string label;
    std::vector<uint8_t> id;
};

struct Pkcs11Config {
    std::string module_path;
    CK_SLOT_ID slot = 0;
    std::string pin;
    Pkcs11KeySelector aes_key;
    Pkcs11KeySelector verify_key;
};

class Pkcs11Engine : public CryptoEngine {
public:
    explicit Pkcs11Engine(Pkcs11Config config);
    ~Pkcs11Engine() override;

    std::string name() const override { return "pkcs11"; }
    std::string last_error() const override { return last_error_; }

    bool ready() const { return ready_; }

    bool DecryptBegin(const std::vector<uint8_t>& iv) override;
    bool DecryptUpdate(const uint8_t* in, size_t in_len,
                       std::vector<uint8_t>& out) override;
    bool DecryptFinish(std::vector<uint8_t>& out) override;

    bool VerifyBegin() override;
    bool VerifyUpdate(const uint8_t* data, size_t len) override;
    bool VerifyFinal(const std::vector<uint8_t>& signature) override;

private:
    bool Initialize();
    bool CheckMechanism(CK_MECHANISM_TYPE mechanism, CK_FLAGS operation);
    bool FindKey(CK_OBJECT_CLASS object_class, CK_KEY_TYPE key_type,
                 const Pkcs11KeySelector& selector,
                 CK_OBJECT_HANDLE& object_handle);
    bool SetError(const std::string& operation, CK_RV rv);
    bool ConvertSignature(const std::vector<uint8_t>& signature,
                          std::vector<uint8_t>& raw_signature);
    bool ProcessCiphertext(const uint8_t* data, size_t len,
                           std::vector<uint8_t>& out);
    bool AppendDecrypted(const uint8_t* data, size_t len,
                         std::vector<uint8_t>& out);

    Pkcs11Config config_;
    void* module_handle_ = nullptr;
    CK_FUNCTION_LIST_PTR functions_ = nullptr;
    CK_SESSION_HANDLE session_ = CK_INVALID_HANDLE;
    CK_OBJECT_HANDLE aes_key_ = CK_INVALID_HANDLE;
    CK_OBJECT_HANDLE verify_key_ = CK_INVALID_HANDLE;
    bool initialized_ = false;
    bool logged_in_ = false;
    bool ready_ = false;
    std::string last_error_;
    std::vector<uint8_t> pending_ciphertext_;
    std::vector<uint8_t> final_plaintext_block_;
};

} // namespace imx93
