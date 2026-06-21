# imx93-edge-lab: Main System Specification

## 1. System Overview
The `imx93-edge-lab` repository provides a comprehensive toolkit for benchmarking, profiling, and deploying AI models on the NXP i.MX93 processor.

## 2. Component Specifications
This main specification delegates detailed technical requirements to module-specific documents:

*   **AI Benchmarks:** `ai-cpp-benchmarks/AI_SPEC.md` - Details the C++ architecture for CPU vs NPU inference latency measurement.
*   **Feature Modules:** Any new feature (e.g., Power Monitoring, Device Tree Overlays) must have its own `feature.md` file in its respective directory.
*   **Subproject Specs:** Each major subproject should also maintain a dedicated `SPEC.md` in its own folder, as with `ai-cpp-benchmarks/AI_SPEC.md`.

## 3. Hardware Target
*   **Board:** NXP i.MX93 FRDM board
*   **CPU:** ARM Cortex-A55
*   **NPU:** ARM Ethos-U65
*   **OS:** Yocto Linux (fsl-imx-xwayland)

## 4. Feature Development Rules
When starting work on a new phase or module:
1. Copy `FEATURE_TEMPLATE.md` to a `feature.md` inside the module's directory (e.g., `power-management/feature.md`).
2. Define the Purpose, Scope, Architecture, and Roadmap for that feature.
3. Update the AI Assistant Instructions (in `.cursorrules` and `.github/copilot-instructions.md`) to reference the new `feature.md` while working on it.