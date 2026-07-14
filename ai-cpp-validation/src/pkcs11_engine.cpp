#include "pkcs11_engine.hpp"

#include <algorithm>
#include <cstring>
#include <dlfcn.h>
#include <iomanip>
#include <sstream>
#include <utility>

#include <openssl/bn.h>
#include <openssl/ecdsa.h>

namespace imx93 {

namespace {

constexpr size_t kAesBlockSize = 16;
constexpr size_t kP256CoordinateSize = 32;

const char* Pkcs11ErrorName(CK_RV rv) {
    switch (rv) {
    case CKR_OK:
        return "CKR_OK";
    case CKR_ARGUMENTS_BAD:
        return "CKR_ARGUMENTS_BAD";
    case CKR_BUFFER_TOO_SMALL:
        return "CKR_BUFFER_TOO_SMALL";
    case CKR_CRYPTOKI_NOT_INITIALIZED:
        return "CKR_CRYPTOKI_NOT_INITIALIZED";
    case CKR_DATA_LEN_RANGE:
        return "CKR_DATA_LEN_RANGE";
    case CKR_DEVICE_ERROR:
        return "CKR_DEVICE_ERROR";
    case CKR_ENCRYPTED_DATA_LEN_RANGE:
        return "CKR_ENCRYPTED_DATA_LEN_RANGE";
    case CKR_FUNCTION_NOT_SUPPORTED:
        return "CKR_FUNCTION_NOT_SUPPORTED";
    case CKR_KEY_FUNCTION_NOT_PERMITTED:
        return "CKR_KEY_FUNCTION_NOT_PERMITTED";
    case CKR_KEY_HANDLE_INVALID:
        return "CKR_KEY_HANDLE_INVALID";
    case CKR_KEY_TYPE_INCONSISTENT:
        return "CKR_KEY_TYPE_INCONSISTENT";
    case CKR_MECHANISM_INVALID:
        return "CKR_MECHANISM_INVALID";
    case CKR_MECHANISM_PARAM_INVALID:
        return "CKR_MECHANISM_PARAM_INVALID";
    case CKR_OBJECT_HANDLE_INVALID:
        return "CKR_OBJECT_HANDLE_INVALID";
    case CKR_OPERATION_ACTIVE:
        return "CKR_OPERATION_ACTIVE";
    case CKR_OPERATION_NOT_INITIALIZED:
        return "CKR_OPERATION_NOT_INITIALIZED";
    case CKR_PIN_INCORRECT:
        return "CKR_PIN_INCORRECT";
    case CKR_SESSION_HANDLE_INVALID:
        return "CKR_SESSION_HANDLE_INVALID";
    case CKR_SIGNATURE_INVALID:
        return "CKR_SIGNATURE_INVALID";
    case CKR_SIGNATURE_LEN_RANGE:
        return "CKR_SIGNATURE_LEN_RANGE";
    case CKR_SLOT_ID_INVALID:
        return "CKR_SLOT_ID_INVALID";
    case CKR_USER_NOT_LOGGED_IN:
        return "CKR_USER_NOT_LOGGED_IN";
    default:
        return "CKR_UNKNOWN";
    }
}

} // namespace

Pkcs11Engine::Pkcs11Engine(Pkcs11Config config) : config_(std::move(config)) {
    ready_ = Initialize();
}

Pkcs11Engine::~Pkcs11Engine() {
    if (functions_ != nullptr) {
        if (logged_in_ && session_ != CK_INVALID_HANDLE) {
            functions_->C_Logout(session_);
        }
        if (session_ != CK_INVALID_HANDLE) {
            functions_->C_CloseSession(session_);
        }
        if (initialized_) {
            functions_->C_Finalize(nullptr);
        }
    }
    if (module_handle_ != nullptr) {
        dlclose(module_handle_);
    }
}

bool Pkcs11Engine::SetError(const std::string& operation, CK_RV rv) {
    std::ostringstream message;
    message << operation << " failed (" << Pkcs11ErrorName(rv)
            << ", CK_RV=0x" << std::hex << std::uppercase << rv << ")";
    last_error_ = message.str();
    return false;
}

bool Pkcs11Engine::CheckMechanism(CK_MECHANISM_TYPE mechanism,
                                  CK_FLAGS operation) {
    CK_MECHANISM_INFO info{};
    CK_RV rv = functions_->C_GetMechanismInfo(config_.slot, mechanism, &info);
    if (rv != CKR_OK) {
        std::ostringstream operation_name;
        operation_name << "C_GetMechanismInfo(mechanism=0x" << std::hex
                       << std::uppercase << mechanism << ")";
        return SetError(operation_name.str(), rv);
    }
    if ((info.flags & operation) == 0) {
        std::ostringstream message;
        message << "PKCS#11 mechanism 0x" << std::hex << std::uppercase
                << mechanism << " does not support the required operation";
        last_error_ = message.str();
        return false;
    }
    return true;
}

bool Pkcs11Engine::FindKey(CK_OBJECT_CLASS object_class, CK_KEY_TYPE key_type,
                           const Pkcs11KeySelector& selector,
                           CK_OBJECT_HANDLE& object_handle) {
    if (selector.label.empty() && selector.id.empty()) {
        last_error_ = "PKCS#11 key selector requires a label or ID";
        return false;
    }

    std::vector<CK_ATTRIBUTE> attributes = {
        {CKA_CLASS, &object_class, sizeof(object_class)},
        {CKA_KEY_TYPE, &key_type, sizeof(key_type)},
    };
    if (!selector.label.empty()) {
        attributes.push_back({CKA_LABEL,
                              const_cast<char*>(selector.label.data()),
                              static_cast<CK_ULONG>(selector.label.size())});
    }
    if (!selector.id.empty()) {
        attributes.push_back({CKA_ID,
                              const_cast<uint8_t*>(selector.id.data()),
                              static_cast<CK_ULONG>(selector.id.size())});
    }

    CK_RV rv = functions_->C_FindObjectsInit(
        session_, attributes.data(), static_cast<CK_ULONG>(attributes.size()));
    if (rv != CKR_OK) {
        return SetError("C_FindObjectsInit", rv);
    }

    CK_OBJECT_HANDLE matches[2] = {CK_INVALID_HANDLE, CK_INVALID_HANDLE};
    CK_ULONG count = 0;
    rv = functions_->C_FindObjects(session_, matches, 2, &count);
    CK_RV final_rv = functions_->C_FindObjectsFinal(session_);
    if (rv != CKR_OK) {
        return SetError("C_FindObjects", rv);
    }
    if (final_rv != CKR_OK) {
        return SetError("C_FindObjectsFinal", final_rv);
    }
    if (count != 1) {
        std::ostringstream message;
        message << "PKCS#11 key selector matched " << count
                << " objects; expected exactly one";
        last_error_ = message.str();
        return false;
    }

    object_handle = matches[0];
    return true;
}

bool Pkcs11Engine::Initialize() {
    module_handle_ = dlopen(config_.module_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (module_handle_ == nullptr) {
        const char* error = dlerror();
        last_error_ = "Failed to load PKCS#11 module " + config_.module_path;
        if (error != nullptr) {
            last_error_ += ": ";
            last_error_ += error;
        }
        return false;
    }

    using GetFunctionList = CK_RV (*)(CK_FUNCTION_LIST_PTR_PTR);
    auto get_function_list = reinterpret_cast<GetFunctionList>(
        dlsym(module_handle_, "C_GetFunctionList"));
    if (get_function_list == nullptr) {
        last_error_ = "PKCS#11 module does not export C_GetFunctionList";
        return false;
    }

    CK_RV rv = get_function_list(&functions_);
    if (rv != CKR_OK || functions_ == nullptr) {
        return SetError("C_GetFunctionList", rv);
    }

    rv = functions_->C_Initialize(nullptr);
    if (rv == CKR_OK) {
        initialized_ = true;
    } else if (rv != CKR_CRYPTOKI_ALREADY_INITIALIZED) {
        return SetError("C_Initialize", rv);
    }

    CK_ULONG slot_count = 0;
    rv = functions_->C_GetSlotList(CK_TRUE, nullptr, &slot_count);
    if (rv != CKR_OK) {
        return SetError("C_GetSlotList", rv);
    }
    std::vector<CK_SLOT_ID> slots(slot_count);
    rv = functions_->C_GetSlotList(CK_TRUE, slots.data(), &slot_count);
    if (rv != CKR_OK) {
        return SetError("C_GetSlotList", rv);
    }
    if (std::find(slots.begin(), slots.end(), config_.slot) == slots.end()) {
        last_error_ = "Requested PKCS#11 slot is not present";
        return false;
    }

    if (!CheckMechanism(CKM_AES_CBC, CKF_DECRYPT) ||
        !CheckMechanism(CKM_ECDSA_SHA256, CKF_VERIFY)) {
        return false;
    }

    rv = functions_->C_OpenSession(config_.slot,
                                   CKF_SERIAL_SESSION | CKF_RW_SESSION,
                                   nullptr, nullptr, &session_);
    if (rv != CKR_OK) {
        return SetError("C_OpenSession", rv);
    }

    CK_UTF8CHAR_PTR pin = config_.pin.empty()
                              ? nullptr
                              : reinterpret_cast<CK_UTF8CHAR_PTR>(
                                    const_cast<char*>(config_.pin.data()));
    rv = functions_->C_Login(session_, CKU_USER, pin,
                             static_cast<CK_ULONG>(config_.pin.size()));
    if (rv == CKR_OK) {
        logged_in_ = true;
    } else if (rv != CKR_USER_ALREADY_LOGGED_IN) {
        return SetError("C_Login", rv);
    }

    if (!FindKey(CKO_SECRET_KEY, CKK_AES, config_.aes_key, aes_key_) ||
        !FindKey(CKO_PUBLIC_KEY, CKK_EC, config_.verify_key, verify_key_)) {
        return false;
    }

    return true;
}

bool Pkcs11Engine::DecryptBegin(const std::vector<uint8_t>& iv) {
    last_error_.clear();
    pending_ciphertext_.clear();
    final_plaintext_block_.clear();
    if (!ready_) {
        return false;
    }
    if (iv.size() != kIvLength) {
        last_error_ = "AES-CBC IV must be 16 bytes";
        return false;
    }

    CK_MECHANISM mechanism{CKM_AES_CBC, const_cast<uint8_t*>(iv.data()),
                           static_cast<CK_ULONG>(iv.size())};
    CK_RV rv = functions_->C_DecryptInit(session_, &mechanism, aes_key_);
    if (rv != CKR_OK) {
        return SetError("C_DecryptInit", rv);
    }
    return true;
}

bool Pkcs11Engine::AppendDecrypted(const uint8_t* data, size_t len,
                                   std::vector<uint8_t>& out) {
    if (len == 0) {
        return true;
    }
    if (len % kAesBlockSize != 0) {
        last_error_ = "PKCS#11 AES-CBC returned a partial plaintext block";
        return false;
    }

    if (!final_plaintext_block_.empty()) {
        out.insert(out.end(), final_plaintext_block_.begin(),
                   final_plaintext_block_.end());
    }
    out.insert(out.end(), data, data + len - kAesBlockSize);
    final_plaintext_block_.assign(data + len - kAesBlockSize, data + len);
    return true;
}

bool Pkcs11Engine::ProcessCiphertext(const uint8_t* data, size_t len,
                                     std::vector<uint8_t>& out) {
    std::vector<uint8_t> decrypted(len + kAesBlockSize);
    CK_ULONG decrypted_len = static_cast<CK_ULONG>(decrypted.size());
    CK_RV rv = functions_->C_DecryptUpdate(
        session_, const_cast<uint8_t*>(data), static_cast<CK_ULONG>(len),
        decrypted.data(), &decrypted_len);
    if (rv != CKR_OK) {
        return SetError("C_DecryptUpdate", rv);
    }
    return AppendDecrypted(decrypted.data(), decrypted_len, out);
}

bool Pkcs11Engine::DecryptUpdate(const uint8_t* in, size_t in_len,
                                 std::vector<uint8_t>& out) {
    out.clear();
    pending_ciphertext_.insert(pending_ciphertext_.end(), in, in + in_len);
    size_t process_len =
        (pending_ciphertext_.size() / kAesBlockSize) * kAesBlockSize;
    if (process_len == 0) {
        return true;
    }

    if (!ProcessCiphertext(pending_ciphertext_.data(), process_len, out)) {
        return false;
    }
    pending_ciphertext_.erase(pending_ciphertext_.begin(),
                              pending_ciphertext_.begin() + process_len);
    return true;
}

bool Pkcs11Engine::DecryptFinish(std::vector<uint8_t>& out) {
    out.clear();
    if (!pending_ciphertext_.empty()) {
        last_error_ = "AES-CBC ciphertext length is not a multiple of 16 bytes";
        return false;
    }

    std::vector<uint8_t> final_bytes(kAesBlockSize);
    CK_ULONG final_len = static_cast<CK_ULONG>(final_bytes.size());
    CK_RV rv = functions_->C_DecryptFinal(session_, final_bytes.data(),
                                          &final_len);
    if (rv != CKR_OK) {
        return SetError("C_DecryptFinal", rv);
    }
    if (!AppendDecrypted(final_bytes.data(), final_len, out)) {
        return false;
    }
    if (final_plaintext_block_.size() != kAesBlockSize) {
        last_error_ = "Decrypted model is missing its final PKCS#7 block";
        return false;
    }

    uint8_t padding = final_plaintext_block_.back();
    if (padding == 0 || padding > kAesBlockSize ||
        !std::all_of(final_plaintext_block_.end() - padding,
                     final_plaintext_block_.end(),
                     [padding](uint8_t value) { return value == padding; })) {
        last_error_ = "Invalid PKCS#7 padding after PKCS#11 decryption";
        return false;
    }

    out.insert(out.end(), final_plaintext_block_.begin(),
               final_plaintext_block_.end() - padding);
    final_plaintext_block_.clear();
    return true;
}

bool Pkcs11Engine::VerifyBegin() {
    last_error_.clear();
    if (!ready_) {
        return false;
    }
    CK_MECHANISM mechanism{CKM_ECDSA_SHA256, nullptr, 0};
    CK_RV rv = functions_->C_VerifyInit(session_, &mechanism, verify_key_);
    if (rv != CKR_OK) {
        return SetError("C_VerifyInit", rv);
    }
    return true;
}

bool Pkcs11Engine::VerifyUpdate(const uint8_t* data, size_t len) {
    if (len == 0) {
        return true;
    }
    CK_RV rv = functions_->C_VerifyUpdate(
        session_, const_cast<uint8_t*>(data), static_cast<CK_ULONG>(len));
    if (rv != CKR_OK) {
        return SetError("C_VerifyUpdate", rv);
    }
    return true;
}

bool Pkcs11Engine::ConvertSignature(
    const std::vector<uint8_t>& signature,
    std::vector<uint8_t>& raw_signature) {
    if (signature.size() == 2 * kP256CoordinateSize) {
        raw_signature = signature;
        return true;
    }

    const unsigned char* cursor = signature.data();
    ECDSA_SIG* parsed = d2i_ECDSA_SIG(nullptr, &cursor, signature.size());
    if (parsed == nullptr ||
        cursor != signature.data() + signature.size()) {
        ECDSA_SIG_free(parsed);
        last_error_ = "Signature is neither raw P-256 R||S nor DER ECDSA";
        return false;
    }

    const BIGNUM* r = nullptr;
    const BIGNUM* s = nullptr;
    ECDSA_SIG_get0(parsed, &r, &s);
    raw_signature.assign(2 * kP256CoordinateSize, 0);
    bool valid = r != nullptr && s != nullptr &&
                 BN_num_bytes(r) <= static_cast<int>(kP256CoordinateSize) &&
                 BN_num_bytes(s) <= static_cast<int>(kP256CoordinateSize) &&
                 BN_bn2binpad(r, raw_signature.data(), kP256CoordinateSize) ==
                     static_cast<int>(kP256CoordinateSize) &&
                 BN_bn2binpad(s,
                              raw_signature.data() + kP256CoordinateSize,
                              kP256CoordinateSize) ==
                     static_cast<int>(kP256CoordinateSize);
    ECDSA_SIG_free(parsed);
    if (!valid) {
        last_error_ = "Invalid ECDSA P-256 signature values";
        return false;
    }
    return true;
}

bool Pkcs11Engine::VerifyFinal(const std::vector<uint8_t>& signature) {
    std::vector<uint8_t> raw_signature;
    if (!ConvertSignature(signature, raw_signature)) {
        return false;
    }

    CK_RV rv = functions_->C_VerifyFinal(
        session_, raw_signature.data(),
        static_cast<CK_ULONG>(raw_signature.size()));
    if (rv == CKR_OK) {
        return true;
    }
    if (rv == CKR_SIGNATURE_INVALID || rv == CKR_SIGNATURE_LEN_RANGE) {
        last_error_ = "PKCS#11 signature verification failed";
        return false;
    }
    return SetError("C_VerifyFinal", rv);
}

} // namespace imx93
