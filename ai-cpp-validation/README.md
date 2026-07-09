# ai-cpp-validation

This repository contains the C++ validation runtime for measuring edge AI performance and implementing secure model onboarding pipelines on the FRDM-iMX93 platform (Yocto 6.6 Scarthgap).

## Contents
- `CMakeLists.txt`
- `feature.md`
- `host-crypto-setup.md`
- `ele-hardware-guide.md`
- `src/`

## Goal
Decrypt and verify the digital signature of a TensorFlow Lite model on the i.MX93 platform, then report execution and latency results.

---

## Cryptographic Setup Pipelines

To prevent repository clutter, the detailed step-by-step cryptographic workflows are split into dedicated developer guides:

* **[Host Cryptographic Pipeline Setup Guide](host-crypto-setup.md):** Steps for offline key generation, model signing, and asset encryption using OpenSSL on your local machine.
* **[Target EdgeLock Enclave (ELE) Hardware Guide](ele-hardware-guide.md):** Target cross-compilation SDK paths, key storage layout, and provisioning strategies for interacting with the hardware enclave.

---

## Build Instructions

### 1. Signing and Encryption on Host (WSL2 / Ubuntu)
Before deploying to the target, the model must be prepared via OpenSSL. Refer to **[host-crypto-setup.md](host-crypto-setup.md)** for the raw command definitions, or use the helper that runs the whole pipeline (key generation, encryption, signing):

```bash
./scripts/prepare_model.sh path/to/model.tflite out_dir/
```

### 2. Build the Runtime
```bash
mkdir build && cd build
# Native/host build (software OpenSSL engine only):
cmake ..
make
# Cross-compile for i.MX93 with the EdgeLock Enclave engine (Phase 3.2):
#   source /opt/fsl-imx-xwayland/<ver>/environment-setup-armv8a-poky-linux
#   cmake -DCMAKE_TOOLCHAIN_FILE=$OECORE_NATIVE_SYSROOT/usr/share/cmake/OEToolchainConfig.cmake -DENABLE_IMX_ELE=ON ..
#   make
```

### 3. Decryption and Verification Engines on i.MX93 Hardware
The runtime application supports two independent cryptographic backends selected via execution arguments:

#### Option 1: Using OpenSSL (`--engine openssl`)
* **Security Level:** Insecure (For local development/validation only).
* **Behavior:** Plaintext cryptographic keys are loaded and managed inside the Linux user-space application memory space. Used primarily to verify user-space file reading, chunk parsing, and core verification logic.

#### Option 2: Using EdgeLock Enclave (`--engine ele`)
* **Status:** Not yet implemented — planned for Phase 3.2. Selecting `--engine ele` currently reports a clear "not yet implemented" message and exits non-zero.
* **Security Level:** Hardened / Secure.
* **Behavior:** Keys are loaded and processed strictly within the isolated hardware security boundary of the EdgeLock Secure Enclave (ELE). Plaintext key material is never exposed to Linux user-space memory or DDR.
* **Prerequisites:** Cryptographic key assets must be provisioned or securely imported into the hardware enclave keystore before running. See **[ele-hardware-guide.md](ele-hardware-guide.md)** for configuration details.

---

## Usage

Copy the compiled runtime executable, your encrypted `.tflite` models (compiled with Vela for Ethos-U65), and the detached `.sig` signature file to the target board partition.

### CLI Syntax
```bash
Usage: ai-cpp-validation [OPTIONS]

Options:
  --model <path>       Path to the encrypted model file (.tflite.enc)
  --signature <path>   Path to the detached signature file (.sig)
  --engine <name>      Cryptographic engine selection: 'openssl' or 'ele'
  --aes-key <path>     [openssl] Hex-encoded AES-256 key file
  --public-key <path>  [openssl] ECDSA public key (PEM)
  --out <path>         Optional: write the decrypted model if verification passes
  --chunk-size <N>     Optional: streaming chunk size in bytes (default 65536)
```

### Example (OpenSSL engine)
```bash
ai-cpp-validation --engine openssl \
  --model model.tflite.enc \
  --signature model.tflite.sig \
  --aes-key model_aes.key \
  --public-key model_public_key.pem
```

The tool exits `0` when the signature verifies (integrity **PASS**) and non-zero otherwise.