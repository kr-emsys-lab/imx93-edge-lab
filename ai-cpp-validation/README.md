# AI C++ Model Validation

Phase 3 validates an encrypted TFLite model before inference. The runtime decrypts AES-256-CBC ciphertext, verifies an ECDSA P-256/SHA-256 signature over the plaintext, and reports cryptographic latency.

Two engines use the same encrypted model and detached signature:

- `openssl`: development baseline using an AES key file and PEM public key.
- `pkcs11`: NXP EdgeLock path using provisioned objects through the SMW PKCS#11 module.

`--engine ele` remains an alias for `--engine pkcs11` for compatibility with the earlier design.

## Asset Format

```text
model.tflite.enc = [16-byte IV][AES-256-CBC ciphertext with PKCS#7 padding]
model.tflite.sig = ECDSA P-256/SHA-256 signature over plaintext
```

The existing OpenSSL script creates a DER-encoded ECDSA signature. The PKCS#11 engine converts it to the raw 64-byte P-256 `R || S` form required by `CKM_ECDSA_SHA256`.

## Prepare Development Assets

Run this on the host, not on the target:

```bash
cd ai-cpp-validation
./scripts/prepare_model.sh /path/to/model.tflite output/
```

Generated files include:

- `model.tflite.enc`
- `model.tflite.sig`
- `model_aes.key`
- `model_private_key.pem`
- `model_public_key.pem`

The private key and raw AES key are development/provisioning assets and must not be copied to the board for PKCS#11 operation. The provisioned AES object must contain the key used to create `.enc`, and the provisioned EC public-key object must correspond to the private key used to create `.sig`.

## Native OpenSSL Build

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/ai-cpp-validation \
  --engine openssl \
  --model output/model.tflite.enc \
  --signature output/model.tflite.sig \
  --aes-key output/model_aes.key \
  --public-key output/model_public_key.pem
```

Use `--out verified-model.tflite` only when a plaintext file is required for development. Without `--out`, verified plaintext remains in process memory and is not written to storage.

## SMW PKCS#11 Build for i.MX93

Source the NXP Yocto SDK first:

```bash
source /opt/fsl-imx-xwayland/6.6-nanbield/environment-setup-armv8a-poky-linux
```

Then cross-compile:

```bash
cmake -S . -B build-pkcs11 \
  -DCMAKE_TOOLCHAIN_FILE=$OECORE_NATIVE_SYSROOT/usr/share/cmake/OEToolchainConfig.cmake \
  -DENABLE_SMW_PKCS11=ON
cmake --build build-pkcs11
```

CMake searches the SDK sysroot for `pkcs11.h`, including an `smw_pkcs11` suffix. If the SDK uses a different location, pass it explicitly:

```bash
-DSMW_PKCS11_INCLUDE_DIR=/path/to/sdk/sysroot/usr/include/smw_pkcs11
```

The executable links `libdl` and loads the target module at runtime. It does not link directly against the SMW module.

## Run with Provisioned ELE Objects

Default SMW module:

```text
/usr/lib/libsmw_pkcs11.so.5
```

Select objects by label:

```bash
./ai-cpp-validation \
  --engine pkcs11 \
  --model model.tflite.enc \
  --signature model.tflite.sig \
  --pkcs11-slot 0 \
  --aes-key-label MyAESKey \
  --verify-key-label MyECCKey
```

Select objects by hexadecimal `CKA_ID`:

```bash
./ai-cpp-validation \
  --engine pkcs11 \
  --model model.tflite.enc \
  --signature model.tflite.sig \
  --aes-key-id 01 \
  --verify-key-id 02
```

A label and ID may both be supplied for stricter matching. Each selector must identify exactly one object. Override the module when needed with:

```bash
--pkcs11-module /usr/lib/libsmw_pkcs11.so.5
```

SMW does not require a PIN. `--pkcs11-pin` exists only for compatibility with PKCS#11 providers that require one; avoid placing sensitive PINs directly in shell history or process listings.

## Required PKCS#11 Objects

AES object:

- `CKO_SECRET_KEY`
- `CKK_AES`
- AES-256
- decryption enabled for `CKM_AES_CBC`

Verification object:

- `CKO_PUBLIC_KEY`
- `CKK_EC`
- P-256 public key
- verification enabled for `CKM_ECDSA_SHA256`

Object provisioning is intentionally separate from this application. The runtime never creates, imports, overwrites, or deletes ELE objects.

## Other Options

```text
--out <path>          write verified plaintext for development
--chunk-size <bytes>  streaming input size; default 65536
```

The chunk size may be any positive value. The PKCS#11 backend internally aligns calls to AES block boundaries.

## Security Notes

- The OpenSSL engine is a functional baseline, not production key protection.
- PKCS#11 mode never loads a raw AES key or private signing key into the application.
- The EC verification key is public but remains managed as a provisioned PKCS#11 object.
- Signature verification occurs over decrypted plaintext.
- Plaintext is released to `--out` only after signature verification passes.
