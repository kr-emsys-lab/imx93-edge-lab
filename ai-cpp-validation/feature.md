# Feature Description: Secure Model Protection & Target Verification

## 1. Purpose
The purpose of this feature is to transform the `ai-cpp-validation` tool from a pure performance benchmark into a hardened execution runtime. It ensures the **Confidentiality** (Intellectual Property protection via AES decryption) and **Integrity** (Authenticity verification via digital signatures) of AI models before they are loaded into memory or passed to the NPU/CPU for inference.

## 2. Scope
* **In Scope:**
  * Host-side scripts for model encryption (AES-256-CBC/CTR) and signing (ECDSA P-256 / SHA-256) via OpenSSL.
  * Target C++ application support for streaming/chunk-based model decryption.
  * Dual-engine verification architecture controlled via command-line arguments (`--engine openssl` vs `--engine ele`).
  * Explicit command-line parameters to accept the encrypted model path and its detached signature path.
  * Hardware-accelerated block decryption loop inside the C++ runtime via the NXP EdgeLock Secure Enclave (ELE) Cipher Service (`hsm_cipher_one_go`) for demonstration purposes.
  * Hardware-anchored signature verification utilizing the ELE APIs (`hsm_verify_signature`).
* **Out of Scope:**
  * System-wide platform integration like secure boot (AHAB) or read-only rootfs integrity (`dm-verity`), which are deferred to **Phase 4**.

## 3. Architecture

[ Params: --model <path> --signature <path> ]
                                  │
                                  ▼
                   ┌─────────────────────────────┐
                   │   Command Line Argument    │
                   │   --engine <openssl|ele>    │
                   └──────────────┬──────────────┘
                                  │
           ┌──────────────────────┴──────────────────────┐
           ▼                                             ▼
 [ Engine: openssl ]                             [ Engine: ele ]
┌───────────────────────────┐                 ┌───────────────────────────┐
│ User-space Decryption     │                 │ Hardware Decryption Loop  │
│ (Chunks via LibCrypto)    │                 │ (Chunks via ELE Cipher    │
├───────────────────────────┤                 │  Service: hsm_cipher_one_go)
│ Hash Generation (SHA256)  │                 ├───────────────────────────┤
├───────────────────────────┤                 │ Hash Generation (SHA256)  │
│ Software Verification     │                 ├───────────────────────────┤
│ Passes local signature &  │                 │ Hardware Verification     │
│ public_key.pem to OpenSSL │                 │ Passes signature & hash   │
└───────────────────────────┘                 │ context via hsm_verify    │
│                                └───────────────────────────┘
│                                             │
└──────────────────────┬──────────────────────┘
│
▼
[ Integrity Pass / Fail ]
│
(If Pass: Load to NPU for Inference)

### Key Management Design
* **Software Mode:** Expects a raw standard symmetric AES key and asymmetric `public_key.pem` on the local file system alongside the runtime arguments.
* **Hardware Mode:** Expects pre-provisioned or securely imported transient RAM Key Slot IDs inside the ELE Keystore (both for the AES decryption key and the ECDSA public key). Raw key material is never exposed to Linux user-space memory.

## 4. Implementation Roadmap

### Sub-Phase 3.1: Offline Tooling & Software Engine Baseline (OpenSSL Host & HW)
* **Goal:** Establish the crypto pipeline and verification logic without hardware drivers.
* **Tasks:**
  * Design an offline tooling pipeline (Bash/Python scripts) to encrypt TFLite models via AES-256 and generate separate `.sig` signature files.
  * Integrate standard OpenSSL (`libcrypto`) into the C++ application to parse input parameters, decrypt the model file in chunks, and verify integrity against the provided signature file using local software keys.

### Sub-Phase 3.2: Enclave Engine Integration (ELE on HW)
* **Goal:** Offload both chunk decryption and integrity verification to dedicated secure hardware and analyze performance.
* **Tasks:**
  * Implement a pre-inference verification stage in the C++ runtime to dynamically toggle to the ELE verification engine.
  * Implement an operational loop inside the C++ code to stream encrypted model chunks directly through the ELE Messaging Unit via the `hsm_cipher_one_go` API.
  * Accumulate the resulting plaintext chunk data into a local SHA-256 hashing context before passing the final digest to `hsm_verify_signature`.
  * Extract benchmarks measuring cryptographic performance overhead of the hardware messaging channel before system-wide platform-level locking occurs in Phase 4.

## 5. CMake Requirements

To support the dual-engine design, the `CMakeLists.txt` within the application directory must dynamically find and link both standard cryptographic libraries and the NXP-specific secure enclave libraries if compiled for the target architecture.

### Package & Library Dependencies
* **`OpenSSL::Crypto`:** Required across all platforms for chunk decryption and the software engine validation.
* **`libhsm` / `imx-secure-enclave`:** Required only when compiling for the target system. 

### Minimal Structural Requirements
```cmake
cmake_minimum_required(VERSION 3.16)
project(ai-cpp-validation-security)

# 1. Standard C++ Setup
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Find OpenSSL (LibCrypto is mandatory for chunk decryption)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

# 3. Optional/Conditional target compilation for NXP ELE
option(ENABLE_IMX_ELE "Enable NXP EdgeLock Enclave hardware support" OFF)

if(ENABLE_IMX_ELE)
    # Expecting the Yocto SDK/sysroot to provide the imx-secure-enclave headers and libs
    find_library(IMX_HSM_LIB NAMES hsm REQUIRED)
    include_directories(/usr/include/imx-secure-enclave)
    add_compile_definitions(HAS_EDGE_LOCK_ENCLAVE=1)
endif()

# 4. Target Executable Definition
add_executable(ai-cpp-validation main.cpp)

# 5. Link Libraries
target_link_libraries(ai-cpp-validation PRIVATE OpenSSL::Crypto)

if(ENABLE_IMX_ELE)
    target_link_libraries(ai-cpp-validation PRIVATE ${IMX_HSM_LIB})
endif()