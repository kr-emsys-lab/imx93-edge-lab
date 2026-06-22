# imx93-edge-lab

Welcome to the **imx93-edge-lab** documentation! This project showcases the capabilities of the NXP i.MX93 processor through AI benchmark-driven demos and embedded toolkit workflows.

## Overview
This toolkit provides developer resources to evaluate and utilize the i.MX93 SoC, focusing on comparing the ARM Cortex-A55 against the Ethos-U65 NPU using TensorFlow Lite and the `ethosu_delegate`.

## Core Modules
*   **AI C++ Benchmarks:** A streamlined C++ application to measure inference latency (CPU vs. NPU).
*   **Edge AI Hardware Validation (Phase 2):** Pre-compiled assets and workflows for standard vision models.
*   **Model Protection & Target Verification (Phase 3):** C++ application to to decrypt and verify signature of Models on HW.
*   **Boot2Trust Platform Hardening (Phase 4):** Userspace application isolation to secure the Edge AI runtime execution environment.

## Getting Started
```bash
git clone https://github.com/kr-emsys-lab/imx93-edge-lab.git
cd imx93-edge-lab
```

For detailed build instructions for the c++ benchmark tool, please visit the AI Benchmarks Quick Start.

## Performance Results

Curious about the real-world inference latency difference between the Cortex-A55 CPU and the Ethos-U65 NPU on the i.MX93?

👉 **[View the Hardware Benchmark Results](ai-benchmark-results.md)**

*Maintained by kr-emsys-lab*
