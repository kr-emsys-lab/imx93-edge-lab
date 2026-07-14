# Feature Description: Secure Model Protection & Target Verification

## 1. Purpose

The `ai-cpp-validation` tool protects AI model confidentiality with AES encryption and verifies model authenticity before plaintext is handed to an inference runtime. Phase 3 compares a software OpenSSL path with an NXP EdgeLock-backed path exposed through the SMW PKCS#11 module.

## 2. Scope

### In Scope

- Host-side OpenSSL tooling for AES-256-CBC encryption and ECDSA P-256/SHA-256 signing.
- Target C++ support for streaming model decryption and detached signature verification.
- Runtime selection with `--engine openssl` or `--engine pkcs11`; `ele` remains a compatibility alias for `pkcs11`.
- SMW PKCS#11 integration through `/usr/lib/libsmw_pkcs11.so.5`.
- Selection of provisioned ELE-backed objects by PKCS#11 label and/or `CKA_ID`.
- Decrypt, verify, and total validation latency reporting.

### Out of Scope

- Provisioning or replacing keys in ELE storage.
- Loading a private signing key on the target.
- Running model inference after validation.
- System-wide secure boot, AHAB, or `dm-verity` integration, which is deferred to Phase 4.

## 3. Protected Asset Format

The same files are accepted by both engines:

```text
model.tflite.enc:
  bytes 0..15       random 16-byte AES-CBC IV
  remaining bytes   AES-256-CBC ciphertext with PKCS#7 padding

model.tflite.sig:
  ECDSA P-256 / SHA-256 signature over the plaintext model
```

OpenSSL produces a DER-encoded ECDSA signature. PKCS#11 `CKM_ECDSA_SHA256` expects raw P-256 `R || S`, so the PKCS#11 engine converts DER signatures to two 32-byte big-endian integers before verification. It also accepts an already-raw 64-byte signature.

## 4. Architecture

```text
[ encrypted model + detached signature ]
                    |
                    v
          [ ValidationRunner ]
                    |
          +---------+---------+
          |                   |
          v                   v
 [ OpenSslEngine ]     [ Pkcs11Engine ]
 local AES key file    SMW PKCS#11 module
 public-key PEM        provisioned ELE objects
          |                   |
          +---------+---------+
                    |
          decrypt -> verify
                    |
                    v
       [ PASS + plaintext | FAIL ]
```

The PKCS#11 backend dynamically loads the configured module with `dlopen` and obtains the API through `C_GetFunctionList`. It initializes PKCS#11, validates the requested slot and mechanisms, opens a session, logs in as `CKU_USER`, and locates exactly one AES key and one EC verification key.

SMW uses `CKM_AES_CBC` for this flow. The application therefore retains the final plaintext block, validates PKCS#7 padding after `C_DecryptFinal`, and only then releases the unpadded bytes. Signature verification is streamed through `C_VerifyUpdate` and completed with `C_VerifyFinal` using `CKM_ECDSA_SHA256`.

### Components

- `ValidationConfig`
  - encrypted model and detached signature paths
  - selected engine and chunk size
  - OpenSSL key-file paths
  - PKCS#11 module, slot, optional PIN, and object selectors
  - optional verified plaintext output path
- `ValidationResult`
  - engine
  - integrity result
  - decrypt, verify, and total latency
  - failure message
- `CryptoEngine`
  - abstract streaming decrypt and verify interface
  - optional engine-specific diagnostic through `last_error()`
- `OpenSslEngine : CryptoEngine`
  - AES-256-CBC decryption through OpenSSL EVP
  - ECDSA P-256/SHA-256 verification with a PEM public key
  - development-only raw AES key loading
- `Pkcs11Engine : CryptoEngine`
  - dynamically loads the SMW PKCS#11 module
  - opens and closes the slot/session/login lifecycle
  - discovers AES and EC public-key objects by label and/or ID
  - decrypts through `C_DecryptInit`, `C_DecryptUpdate`, and `C_DecryptFinal`
  - verifies through `C_VerifyInit`, `C_VerifyUpdate`, and `C_VerifyFinal`
  - converts DER ECDSA signatures to raw P-256 `R || S`
- `ModelDecryptor`
  - streams ciphertext through the selected engine
  - accumulates plaintext only for the current validation operation
- `SignatureVerifier`
  - streams decrypted plaintext into the selected verification backend
- `CliReporter`
  - prints integrity result and latency measurements

## 5. PKCS#11 Object Requirements

The board owner provisions the objects separately. The application does not create, import, overwrite, or delete ELE objects.

### AES Decryption Object

- `CKA_CLASS = CKO_SECRET_KEY`
- `CKA_KEY_TYPE = CKK_AES`
- AES-256 key material
- decryption permitted with `CKM_AES_CBC`

### Signature Verification Object

- `CKA_CLASS = CKO_PUBLIC_KEY`
- `CKA_KEY_TYPE = CKK_EC`
- ECDSA P-256 public key
- verification permitted with `CKM_ECDSA_SHA256`

Each object must be selected with a label, an ID, or both. When both are supplied, both attributes must match the same unique object. SMW does not require a PIN, so the default login passes a null, zero-length PIN; an optional PIN is supported for generic PKCS#11 testing.

## 6. Implementation Roadmap

### Sub-Phase 3.1: OpenSSL Baseline

- Generate the encrypted model and detached plaintext signature offline.
- Implement streaming AES-256-CBC decryption.
- Implement ECDSA P-256/SHA-256 verification.
- Validate happy and negative paths on the host and i.MX93.

### Sub-Phase 3.2: SMW PKCS#11 Engine

- Dynamically load the SMW PKCS#11 module.
- Add slot/session/login management and object discovery.
- Add multipart AES-CBC decryption with application-side PKCS#7 validation.
- Add multipart ECDSA-SHA256 verification and DER-to-raw conversion.
- Cross-compile against the target Yocto SDK PKCS#11 headers.

### Sub-Phase 3.3: Hardware Validation

- Run against user-provisioned ELE-backed AES and EC objects.
- Confirm the same encrypted asset passes through both engines.
- Record decrypt, verify, and total hardware latency.
- Confirm raw AES key material and private signing keys do not enter application memory in PKCS#11 mode.

## 7. Build Requirements

OpenSSL is always required. PKCS#11 support is optional at build time and requires the NXP SDK headers `pkcs11.h`, `pkcs11t.h`, and `pkcs11f.h`.

```bash
cmake -S . -B build
cmake --build build

cmake -S . -B build-pkcs11 \
  -DENABLE_SMW_PKCS11=ON \
  -DSMW_PKCS11_INCLUDE_DIR=/path/to/sdk/include/smw_pkcs11
cmake --build build-pkcs11
```

The target SMW library is loaded at runtime rather than linked into the executable.
