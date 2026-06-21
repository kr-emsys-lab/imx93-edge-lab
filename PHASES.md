# Project Phases

## Phase 1: AI Benchmark (Baseline Metrics)
- [x] Create `ai-cpp-benchmarks/` with the performance benchmark tool
- [x] Define developer spec in `ai-cpp-benchmarks/feature.md`
- [x] Publish landing page content in `README.md`
- [x] Validate benchmark tool and models on the host PC
- [ ] Execute the benchmark on the i.MX93 FRDM board and extract baseline metrics

## Phase 2: Edge AI Hardware Validation
- [ ] Provision the i.MX93 target with the stock, unhardened Yocto image
- [ ] Run the dynamic linker verification and the C++ TFLite benchmark via the Ethos-U65 External Delegate
- [ ] Log baseline inference latency, operational execution times, and NPU driver behavior

## Phase 3: Model Protection & Target Verification
- [ ] Design an offline tooling pipeline to encrypt TFLite models via AES-256-GCM and generate SHA-256 integrity signatures
- [ ] Implement a pre-inference verification stage in the C++ runtime to decrypt and verify model signatures on the stock image
- [ ] Interface with the NXP EdgeLock Secure Enclave (via the Linux HSM/PKCS#11 interface or the `ele-apps` / `seco` API) to verify cryptographic performance before system-wide locking

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