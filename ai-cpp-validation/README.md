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
Before deploying to the target, the model must be prepared via OpenSSL. Refer to **[host-crypto-setup.md](host-crypto-setup.md)** for the raw command definitions.

# Host Cryptographic Pipeline Setup

This guide details the offline steps required on the Linux development host machine (WSL2 / Ubuntu) to secure and prepare target AI model assets before deploying them to the i.MX93 hardware.

> ⚠️ **SECURITY DISCLAIMER & PRODUCTION WARNING**
> The local cryptographic operations outlined below are strictly for **proof-of-concept (PoC) and demonstration purposes**. 
> * **The Risk:** Generating and storing raw asymmetric private keys (`model_private_key.pem`) and symmetric deployment keys (`model_aes.key`) on a local development host exposes critical key material to the host environment's file system and memory space.
> * **Production Implementation:** In a production-grade infrastructure, raw key material must never reside on a developer host. The signing and asset-encryption pipeline must be offloaded to a secure, audited environment such as an enterprise Cloud Key Management Service (e.g., AWS KMS, Azure Key Vault, Google Cloud KMS) or a dedicated Hardware Security Module (HSM) integrated into the corporate CI/CD signing pipeline.

---

## 1. Step-by-Step Generation & Asset Packaging

### Step 1: Key Generation
Generate the asymmetric ECDSA key pair used for model signing, and a symmetric AES key for hardware-targeted block decryption.

```bash
# Generate ECDSA Private Key (prime256v1 curve)
openssl ecparam -name prime256v1 -genkey -noout -out model_private_key.pem

# Extract the Public Key (to be deployed/provisioned on the target target)
openssl ec -in model_private_key.pem -pubout -out model_public_key.pem

# Generate a random 256-bit key for AES-256-CBC encryption
openssl rand -hex 32 > model_aes.key


### 2. Decryption and Verification Engines on i.MX93 Hardware
The runtime application supports two independent cryptographic backends selected via execution arguments:

#### Option 1: Using OpenSSL (`--engine openssl`)
* **Security Level:** Insecure (For local development/validation only).
* **Behavior:** Plaintext cryptographic keys are loaded and managed inside the Linux user-space application memory space. Used primarily to verify user-space file reading, chunk parsing, and core verification logic.

#### Option 2: Using EdgeLock Enclave (`--engine ele`)
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
  --model <path>      Path to the encrypted model file (.tflite.enc)
  --signature <path>  Path to the detached signature file (.sig)
  --engine <name>     Cryptographic engine selection: 'openssl' or 'ele'