# Project Phases

## Phase 1: AI Benchmark (Baseline Metrics)
- [x] Create `ai-cpp-benchmarks/` with the performance benchmark tool
- [x] Define developer spec in `ai-cpp-benchmarks/feature.md`
- [x] Publish landing page content in `README.md`
- [x] Validate benchmark tool and models on the host PC
- [x] Execute the benchmark on the i.MX93 FRDM board and extract baseline metrics

## Phase 2: Edge AI Hardware Validation
- [x] Provision the i.MX93 target with the stock, unhardened Yocto image
- [x] Run the dynamic linker verification and the C++ TFLite benchmark via the Ethos-U65 External Delegate
- [x] Log baseline inference latency, operational execution times, and NPU driver behavior

## Phase 3: Model Protection & Target Verification
- [ ] Design an offline tooling pipeline to encrypt TFLite models via AES-256-CBC and generate ECDSA P-256/SHA-256 integrity signatures
- [ ] Implement a pre-inference verification stage in the C++ runtime to decrypt and verify model signatures on the stock image
- [ ] Interface with the NXP EdgeLock Secure Enclave (via the Linux HSM/PKCS#11 interface) using plaintext AES key injection with `pkcs11-tool` for development validation

## Phase 3.5: ELE Secure Key Import (Planned)
- [ ] Implement secure AES-256 model-encryption key provisioning using NXP SPSDK and ELE Secure Import blob generation
- [ ] Build provisioning utility for the i.MX93 target using `libsmw_import_key()` or native `imx-secure-enclave` HSM API
- [ ] Validate persistent key storage via ELE NVM service across reboot cycles
- [ ] Verify PKCS#11 integration with securely imported keys for runtime model decryption
- [ ] Compare performance and security properties of plaintext import (Phase 3) vs. secure import (Phase 3.5) paths
- [ ] Document security boundaries, blob generation workflows, and persistent keystore management

### Detailed Implementation Flow
1. Generate or select the AES-256 model-encryption key on the provisioning host
2. Use NXP SPSDK and i.MX93 Secure Import / Signed Message tooling to prepare the ELE-compatible wrapped key blob or ELE TLV
3. Transfer the generated blob to the i.MX93 target as provisioning input
4. Create a provisioning utility using `libsmw` with `smw_import_key()` or the native `imx-secure-enclave` HSM key-management API
5. Read the blob file into memory and pass the blob buffer to the selected import API
6. Import the key with persistent lifetime, encryption/decryption permissions, required key identifier, and synchronous/persistent NVM semantics
7. Ensure the ELE NVM service/daemon is running so persistent ELE keystore state can be stored in external non-volatile storage
8. Reboot or power-cycle the board and verify that the imported key remains usable after restart
9. Determine whether the securely imported ELE key can be referenced through SMW PKCS#11
10. If PKCS#11 cannot access the imported object, use native `libsmw` or HSM APIs for runtime model decryption
11. Compare: PKCS#11 plaintext key import (Phase 3) vs. ELE Secure Import using `smw_import_key()` vs. native HSM API
12. Document the security boundary, plaintext exposure, blob generation, persistent storage, reboot behavior, and runtime key usage

### Expected Architecture

**Provisioning Flow:**
```
Provisioning host → AES-256 key → NXP SPSDK → Signed/wrapped ELE Secure Import blob
                                                 ↓
                                          Blob file (to i.MX93)
                                                 ↓
                                          Provisioning application
                                                 ↓
                                    Read blob into memory
                                    smw_import_key() or native HSM API
                                                 ↓
                                               ELE
                                                 ↓
                                   Validate and unwrap/import key
                                   Create persistent key object
                                                 ↓
                                    ELE NVM service / nvm_daemon
                                                 ↓
                                   Store protected keystore state
                                                 ↓
                                   External SD/eMMC filesystem
```

**Runtime Flow (after provisioning):**
```
Application → Locate key by identifier → SMW PKCS#11 / native libsmw / native HSM API
                                         ↓
                                        ELE
                                         ↓
                                Perform AES decryption
                                         ↓
                            Decrypted model provided only to runtime
```

### Constraints & Dependencies
- PKCS#11 plaintext key import (Phase 3) must be fully functional and tested as the baseline
- Requires NXP SPSDK toolchain and imx-secure-enclave HSM support on the target
- ELE NVM service must be operational for persistent keystore state
- This phase is prerequisite validation work for Phase 4 (Boot2Trust hardening) to ensure secure key management is viable
- **Do NOT implement Secure Import as part of current Phase 3 PKCS#11 changes** — Phase 3 remains plaintext-key development baseline

## Phase 4: Boot2Trust Platform Hardening (ImmuChain Yocto Integration)
- [ ] Configure NXP Advanced High Assurance Boot (AHAB) managed by the EdgeLock Secure Enclave for hardware-enforced Secure Boot
- [ ] Integrate `dm-verity` into the Yocto Scarthgap (6.6) metadata to enforce rootfs cryptographic integrity
- [ ] Implement userspace application isolation mechanisms to secure the Edge AI runtime execution environment

## Phase 5: Security Validation & Overhead Measurement
- [ ] Flash the fully hardened Boot2Trust system image containing the encrypted models to the physical i.MX93 target
- [ ] Execute the identical C++ TFLite benchmark suite under the performance scaling governor
- [ ] Extract hardened performance metrics and compute the delta against Phase 2 baseline results to isolate the combined overhead of platform hardening and enclave-assisted asset decryption

## Phase 6: Toolkit Release
- [ ] Harden the `imx93-edge-lab` repository architecture for KR-EMSYS-LAB publication
- [ ] Document the Boot2Trust platform security specifications, EdgeLock Enclave key management workflows, and model encryption steps
- [ ] Create GitHub Pages content, performance-vs-security data visualizations, and deployment guides