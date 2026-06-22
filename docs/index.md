# imx93-edge-lab

Welcome to the **imx93-edge-lab** documentation! This project showcases the capabilities of the NXP i.MX93 processor through AI benchmark-driven demos and embedded toolkit workflows.

## Overview
This toolkit provides developer resources to evaluate and utilize the i.MX93 SoC, focusing on comparing the ARM Cortex-A55 against the Ethos-U65 NPU using TensorFlow Lite and the `ethosu_delegate`.

## Core Modules

1. **ai-cpp-benchmarks:** A C++ application to establish baseline AI inference performance on the i.MX93's CPU and NPU. 

    Curious about the real-world inference latency difference between the Cortex-A55 CPU and the Ethos-U65 NPU on the i.MX93?

    👉 **[View the Hardware Benchmark Results](ai-benchmark-results.md)**

2. **ai-cpp-validation:** (TODO) A C++ application demonstrating on-target model decryption and signature verification using the EdgeLock Secure Enclave.
3. **yocto-imx-boot2trust:** (TODO) Custom Yocto layers that integrate Secure Boot (AHAB) and `dm-verity` to create a hardened, immutable system image.

## Getting Started
```bash
git clone https://github.com/kr-emsys-lab/imx93-edge-lab.git
cd imx93-edge-lab
```

For detailed build instructions for the c++ tools, please visit the project Quick Start.

*Maintained by kr-emsys-lab*
