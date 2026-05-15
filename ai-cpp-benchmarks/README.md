# ai-cpp-benchmarks

Phase 1 benchmark module for i.MX93 CPU vs NPU inference latency.

## Contents
- `CMakeLists.txt`
- `SPEC.md`
- `docs/`
- `src/`
- `benchmarks/`
- `models/`
- `platform/`
- `tests/`

## Goal
Measure TensorFlow Lite inference latency on Cortex-A55 and Ethos-U65, then report results in a CLI table.

## Build Instructions

### Option A: Native Build (WSL2 / Ubuntu)
To verify your C++ logic on your host machine without the Ethos-U delegate:

```bash
export TFLITE_DIR=~/project/tensorflow

# 1. Build TFLite C++ library from source
git clone --depth 1 --branch v2.16.1 https://github.com/tensorflow/tensorflow.git ${TFLITE_DIR}
mkdir -p ${TFLITE_DIR}/tflite_build && cd ${TFLITE_DIR}/tflite_build
cmake ../tensorflow/lite
make -j$(nproc)
cd -

# 2. Build the benchmark tool using the newly compiled library
mkdir build && cd build
cmake -DENABLE_NPU=OFF \
  -DTFLITE_INCLUDE_DIR=${TFLITE_DIR} \
  -DTFLITE_LIB=${TFLITE_DIR}/tflite_build/libtensorflow-lite.a \
  -DTFLITE_FLATBUFFERS_DIR=${TFLITE_DIR}/tflite_build/flatbuffers/include \
  -DTFLITE_ABSEIL_DIR=${TFLITE_DIR}/tflite_build/abseil-cpp \
  -DTFLITE_BUILD_DIR=${TFLITE_DIR}/tflite_build ..
make
./imx93_benchmark <model_path> CPU
```

### Option B: Cross-Compile (i.MX93 Hardware)
To build for the NXP i.MX93 with NPU support, use a standard Yocto/OpenEmbedded SDK:

1. Obtain and install the Yocto SDK:
   - **Pre-built:** Download the pre-built Linux SDK installer (`.sh`) for the i.MX93 from the NXP website and execute it.
   - **Build your own:** If you maintain a Yocto project, build the toolchain using `bitbake imx-image-full -c populate_sdk` and run the resulting installer.

2. Source your cross-compilation environment:
   ```bash
   source /opt/fsl-imx-xwayland/.../environment-setup-aarch64-poky-linux
   ```
3. Build the benchmark tool:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

## Usage

Copy the compiled `imx93_benchmark` executable and your `.tflite` models (compiled with Vela for Ethos-U65) to the target board.

```bash
./imx93_benchmark <model_path> [backend: CPU|NPU|ALL]
```

### Example Output

```
+----------------+--------------+--------------------+----------------+
| Model Name     | Backend      | Avg Latency (ms)   | Speedup Factor |
+----------------+--------------+--------------------+----------------+
| mobilenet_v2   | CPU          | 92.50              | 1.00x          |
| mobilenet_v2   | NPU          | 12.40              | 7.46x          |
+----------------+--------------+--------------------+----------------+
```

## Notes
- Project-wide GitHub Pages live in the root `docs/` folder.
- Benchmark-specific documentation lives in `ai-cpp-benchmarks/docs/`.
