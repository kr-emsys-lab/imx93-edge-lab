# imx93-edge-lab

## Mission Statement
The `imx93-edge-lab` is a comprehensive toolkit designed to showcase the capabilities of the NXP i.MX93 processor. It provides developers with ready-to-use AI benchmarks, embedded workflows, and profiling tools to evaluate the ARM Cortex-A55 and Ethos-U65 NPU.

## Current Status
Phase 1 has completed host-side validation for the benchmark tool and model workflows. The i.MX93 FRDM board hardware benchmark execution is still pending.

## Repository Structure
The repository is organized into distinct feature modules. Project-wide documentation is centralized, while feature-specific code and specs are self-contained.

- **`docs/`**: Project-wide GitHub Pages content. Any feature that needs public-facing documentation (like benchmark results or tutorials) gets published here.
- **`ai-cpp-benchmarks/`**: C++ performance benchmark project for CPU vs NPU inference. Contains its own technical spec (`ai-cpp-benchmarks/feature.md`).
- **`ai-cpp-validation/`**: C++ project for OpenSSL vs EdgeLock Secure Enclave model decryptipon and signature verificatione. Contains its own technical spec (`ai-cpp-validation/feature.md`).
- **`scripts/`**: Automation scripts, including the Yocto BSP sync and SDK builder (`build_yocto.sh`). (DRAFT)
- **`SPEC.md` / `PHASES.md`**: Core system architecture rules and project roadmap tracking.

## AI Benchmarks (CPU vs. NPU)
The feature of this toolkit is the `ai-cpp-benchmarks` application. It is a streamlined C++ tool built to measure and compare TensorFlow Lite inference latency directly on the i.MX93 hardware.

## Edge AI Validation (OpenSSL vs. EdgeLock Secure Enclave)
The feature of this toolkit is the `ai-cpp-validation` application. It is a streamlined C++ tool built to decrypt and verify encrypted TensorFlow Lite models directly on the i.MX93 hardware.

**Development Strategy (Phase 3):** Models are encrypted with AES-256-CBC and signed with ECDSA P-256/SHA-256. Development validation uses plaintext key injection via `pkcs11-tool` to establish baseline cryptographic latency on the target.

**Production Strategy (Phase 3.5, Planned):** Secure AES key provisioning via NXP SPSDK and ELE Secure Import blob generation will eliminate plaintext key exposure on the provisioning path. Keys will be securely imported into the EdgeLock Secure Enclave and remain hardware-protected during runtime decryption operations.

See **PHASES.md** for the complete roadmap and technical details of the secure key import flow.

> **Developer Note:** We are actively building out more features! Check out **PHASES.md** to see our internal development roadmap and upcoming milestones (like Vision Model Demos and Power Monitoring).

### Quick Start: Build Environments
You can build this project either natively on your host (for CPU testing) or cross-compile it for the i.MX93 hardware. 

**1. Native Build (Host CPU Only):**
```bash
cd ai-cpp-benchmarks
mkdir build && cd build
cmake -DENABLE_NPU=OFF ..
make
```

See ai-cpp-benchmarks/README.md for detailed compilation instructions and outputs.
