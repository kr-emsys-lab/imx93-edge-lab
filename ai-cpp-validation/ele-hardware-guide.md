# i.MX93 EdgeLock Validation through SMW PKCS#11

This guide covers the hardware backend for `ai-cpp-validation`. The application uses NXP Security Middleware (SMW) PKCS#11 instead of calling `hsm_api.h` directly.

The target module is:

```text
/usr/lib/libsmw_pkcs11.so.5
```

Key provisioning is intentionally not covered. Provision the AES secret key and EC verification public key through the approved SMW/ELE process before running the application.

## 1. Target Prerequisites

Confirm the module exists:

```bash
ls -l /usr/lib/libsmw_pkcs11.so.5
```

Confirm the image's SMW daemon and storage configuration are operational using the same setup already validated with `pkcs11-tool`.

Inspect slots and mechanisms without changing objects:

```bash
pkcs11-tool \
  --module /usr/lib/libsmw_pkcs11.so.5 \
  --list-slots

pkcs11-tool \
  --module /usr/lib/libsmw_pkcs11.so.5 \
  --slot 0 \
  --list-mechanisms
```

The validation flow requires:

```text
CKM_AES_CBC       decrypt
CKM_ECDSA_SHA256  verify
```

SMW examples for i.MX93 use slot `0`, which is also the application default. Override it with `--pkcs11-slot` if the target reports a different slot ID.

## 2. SDK Header Prerequisites

The cross-compilation SDK must provide the PKCS#11 headers:

```text
pkcs11.h
pkcs11t.h
pkcs11f.h
```

They are commonly installed in an SDK sysroot directory such as:

```text
$OECORE_TARGET_SYSROOT/usr/include/smw_pkcs11
```

Locate the actual SDK path before building:

```bash
find "$OECORE_TARGET_SYSROOT/usr/include" \
  -name pkcs11.h -o -name pkcs11t.h -o -name pkcs11f.h
```

The application needs headers only at build time. It loads the target module dynamically at runtime through `dlopen` and `C_GetFunctionList`.

## 3. Required Objects

### AES Decryption Key

The selected object must have:

```text
CKA_CLASS    CKO_SECRET_KEY
CKA_KEY_TYPE CKK_AES
```

It must contain the AES-256 key used by the offline encryption pipeline and permit decryption with `CKM_AES_CBC`.

### ECDSA Verification Key

The selected object must have:

```text
CKA_CLASS    CKO_PUBLIC_KEY
CKA_KEY_TYPE CKK_EC
```

It must contain the P-256 public key corresponding to the offline signing private key and permit verification with `CKM_ECDSA_SHA256`.

The runtime accepts a label, hexadecimal `CKA_ID`, or both for each object. Both selectors are included in the `C_FindObjectsInit` template when both are provided, and exactly one object must match.

The application does not generate, import, modify, or delete objects.

## 4. Cross-Compile

Source the NXP Yocto SDK:

```bash
source /opt/fsl-imx-xwayland/6.6-nanbield/environment-setup-armv8a-poky-linux
```

Build with PKCS#11 support:

```bash
cd ai-cpp-validation
cmake -S . -B build-pkcs11 \
  -DCMAKE_TOOLCHAIN_FILE=$OECORE_NATIVE_SYSROOT/usr/share/cmake/OEToolchainConfig.cmake \
  -DENABLE_SMW_PKCS11=ON
cmake --build build-pkcs11
```

If CMake cannot find the headers automatically:

```bash
cmake -S . -B build-pkcs11 \
  -DCMAKE_TOOLCHAIN_FILE=$OECORE_NATIVE_SYSROOT/usr/share/cmake/OEToolchainConfig.cmake \
  -DENABLE_SMW_PKCS11=ON \
  -DSMW_PKCS11_INCLUDE_DIR="$OECORE_TARGET_SYSROOT/usr/include/smw_pkcs11"
cmake --build build-pkcs11
```

Copy the following to the board:

- `build-pkcs11/ai-cpp-validation`
- encrypted model `.enc`
- detached signature `.sig`

Do not copy `model_aes.key` or `model_private_key.pem` for PKCS#11 execution.

## 5. Run by Object Label

```bash
./ai-cpp-validation \
  --engine pkcs11 \
  --model model.tflite.enc \
  --signature model.tflite.sig \
  --pkcs11-module /usr/lib/libsmw_pkcs11.so.5 \
  --pkcs11-slot 0 \
  --aes-key-label MyAESKey \
  --verify-key-label MyECCKey
```

## 6. Run by Object ID

IDs are supplied as hexadecimal bytes without a `0x` prefix:

```bash
./ai-cpp-validation \
  --engine pkcs11 \
  --model model.tflite.enc \
  --signature model.tflite.sig \
  --aes-key-id 01 \
  --verify-key-id 02
```

A label and ID can be supplied together for each key when stricter matching is required.

SMW does not require a PIN. The application calls `C_Login(session, CKU_USER, NULL_PTR, 0)` by default. `--pkcs11-pin` is available only for generic PKCS#11 providers that require one.

## 7. Runtime Flow

The application performs:

1. `dlopen` of the configured module.
2. `C_GetFunctionList` and `C_Initialize`.
3. `C_GetSlotList` and `C_GetMechanismInfo` validation.
4. `C_OpenSession` and `C_Login`.
5. `C_FindObjectsInit/FindObjects/FindObjectsFinal` for both keys.
6. `C_DecryptInit/Update/Final` with `CKM_AES_CBC`.
7. Application-side PKCS#7 padding validation.
8. `C_VerifyInit/Update/Final` with `CKM_ECDSA_SHA256`.
9. Logout, session close, module finalize, and unload.

Ciphertext is streamed in bounded chunks. The final decrypted block is withheld until all PKCS#7 bytes are validated. Plaintext is written through `--out` only after signature verification passes.

## 8. Signature Compatibility

The host helper signs plaintext with:

```bash
openssl dgst -sha256 -sign model_private_key.pem \
  -out model.tflite.sig model.tflite
```

This output is DER encoded. The PKCS#11 engine parses the DER ECDSA signature and submits raw P-256 `R || S` to SMW. An already-raw 64-byte signature is also accepted.

## 9. Expected Diagnostics

Common failures include:

- module cannot be loaded
- requested slot is not present
- required mechanism is unsupported
- login fails
- no object matches a selector
- more than one object matches a selector
- AES-CBC decrypt operation fails
- PKCS#7 padding is invalid, usually indicating the wrong AES key or corrupted ciphertext
- ECDSA verification fails, usually indicating a mismatched public key, signature, or plaintext

PKCS#11 operation errors include the returned `CK_RV` value to support board-side diagnosis.

## 10. Hardware Validation Record

Capture the command, object labels or non-sensitive IDs, application output, and target image/SDK version. Do not record raw AES keys, private keys, PINs, or other provisioning secrets.
