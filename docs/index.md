# imx93-edge-lab

Welcome to the **imx93-edge-lab** documentation! This project showcases the capabilities of the NXP i.MX93 processor through AI benchmark-driven demos and embedded toolkit workflows.

## Overview
This toolkit provides developer resources to evaluate and utilize the i.MX93 SoC, focusing on comparing the ARM Cortex-A55 against the Ethos-U65 NPU using TensorFlow Lite and the `ethosu_delegate`.

## Core Modules
*   **AI C++ Benchmarks:** A streamlined C++ application to measure inference latency (CPU vs. NPU).
*   **Model Demos (Phase 2):** Pre-compiled assets and workflows for standard vision models.
*   **Platform Integration (Phase 3):** Board-specific device tree overlays and utility helpers.
*   **Power Monitoring (Phase 4):** Latency and energy correlation metrics.

## Getting Started
```bash
git clone https://github.com/KR-EMSYS-LAB/imx93-edge-lab.git
cd imx93-edge-lab
```

For detailed build instructions for the benchmark tool, please visit the AI Benchmarks Quick Start.

## Performance Results

Curious about the real-world inference latency difference between the Cortex-A55 CPU and the Ethos-U65 NPU on the i.MX93?

👉 **[View the Hardware Benchmark Results](results.md)**

*Maintained by kr-emsys-lab*
