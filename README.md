# imx93-eiq-cpp

## Mission Statement
Showcase i.MX93 capability through AI benchmark-driven demos and embedded toolkit workflows.  
Start with phase 1: AI benchmark and GitHub page presentation.

## Repository Structure
- `ai-cpp-benchmarks/`: C++ performance benchmark project for CPU vs NPU inference
- `scripts/`: Python snippets for rapid prototyping and automation
- `device-tree/`: overlay snippets for I2C, SPI, and board configuration
- `power-management/`: scripts and data collection for NPU/CPU power monitoring
- `tools/`: shell scripts for flashing, NXP SPSDK templates, and utility helpers
- `docs/`: project-wide GitHub Pages and common documentation
- `README.md`: landing page for the toolkit

## Phase 1 Focus
Phase 1 (AI Benchmark) is complete! The `ai-cpp-benchmarks` tool provides a streamlined C++ application to measure inference latency on the i.MX93, comparing the ARM Cortex-A55 against the Ethos-U65 NPU.

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
