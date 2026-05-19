# imx93-edge-lab

## Mission Statement
The `imx93-edge-lab` is a comprehensive toolkit designed to showcase the capabilities of the NXP i.MX93 processor. It provides developers with ready-to-use AI benchmarks, embedded workflows, and profiling tools to evaluate the ARM Cortex-A55 and Ethos-U65 NPU.

## Repository Structure
The repository is organized into distinct feature modules. Project-wide documentation is centralized, while feature-specific code and specs are self-contained.

- **`docs/`**: Project-wide GitHub Pages content. Any feature that needs public-facing documentation (like benchmark results or tutorials) gets published here.
- **`ai-cpp-benchmarks/`**: C++ performance benchmark project for CPU vs NPU inference. Contains its own technical spec (`AI_SPEC.md`).
- **`scripts/`**: Automation scripts, including the Yocto BSP sync and SDK builder (`build_yocto.sh`).
- **`device-tree/`**: Overlay snippets for I2C, SPI, and board configuration.
- **`power-management/`**: Scripts and data collection for NPU/CPU power monitoring.
- **`tools/`**: Shell scripts for flashing, NXP SPSDK templates, and utility helpers.
- **`SPEC.md` / `PHASES.md`**: Core system architecture rules and project roadmap tracking.

## AI Benchmarks (CPU vs. NPU)
The flagship feature of this toolkit is the `ai-cpp-benchmarks` application. It is a streamlined C++ tool built to measure and compare TensorFlow Lite inference latency directly on the i.MX93 hardware.

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
