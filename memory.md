# Project Memory

Use this file as a persistent scratchpad to keep track of current context, roadblocks, and next steps between chat sessions.

## Current Status (Phase 1)
- The Yocto build (`scripts/build_yocto.sh`) for the i.MX93 SDK is currently paused and will be resumed later.
- The Native CPU benchmark tool was successfully built and tested.
- Baseline recorded for `mobilenet_v2_1.0_224_quant.tflite` on Cortex-A55 is **14.40 ms**.

## Completed Milestones
- Completed Phase 1 objectives (`PHASES.md`): AI Benchmark tool creation and GitHub Pages setup.
- Set up `ai-cpp-benchmarks/` project with CMake supporting both Native (WSL2/Ubuntu) and Cross-compilation workflows.
- Automated the NXP Yocto BSP sync and SDK generation via `scripts/build_yocto.sh`.
- Configured Jekyll GitHub Pages workflow (`.github/workflows/jekyll-gh-pages.yml`) for serving `docs/`.