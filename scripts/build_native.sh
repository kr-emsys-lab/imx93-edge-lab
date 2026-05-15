#!/bin/bash
set -e

# Resolve absolute paths
WORKSPACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TF_DIR="${WORKSPACE_DIR}/../tensorflow"
BENCHMARK_DIR="${WORKSPACE_DIR}/ai-cpp-benchmarks"
TFLITE_LIB_PATH="${TF_DIR}/tflite_build/libtensorflow-lite.a"

CLEAN_BUILD=0
if [[ "$1" == "--clean" || "$1" == "clean" ]]; then
    CLEAN_BUILD=1
fi

echo "=== Setting up native TFLite build environment ==="

# 1. Clone TensorFlow if it doesn't exist
if [ ! -d "$TF_DIR" ]; then
    echo "Cloning TensorFlow v2.16.1..."
    git clone --depth 1 --branch v2.16.1 https://github.com/tensorflow/tensorflow.git "$TF_DIR"
else
    echo "TensorFlow repository found at $TF_DIR"
fi

# 2. Build TFLite (Skip if already built unless --clean is passed)
if [ "$CLEAN_BUILD" -eq 1 ] || [ ! -f "$TFLITE_LIB_PATH" ]; then
    echo "Configuring and building TensorFlow Lite..."
    rm -rf "${TF_DIR}/tflite_build" # Wipe out any polluted CMake cache
    mkdir -p "${TF_DIR}/tflite_build"
    cd "${TF_DIR}/tflite_build"

    # Configure TFLite, explicitly disabling system packages to force FetchContent (hermetic build)
    cmake ../tensorflow/lite \
      -DCMAKE_DISABLE_FIND_PACKAGE_Flatbuffers=ON \
      -DCMAKE_DISABLE_FIND_PACKAGE_absl=ON

    make -j$(nproc)
else
    echo "TensorFlow Lite library found. Skipping TFLite build."
    echo "Pass '--clean' to force a clean rebuild."
fi

# 3. Build the Benchmark
echo "=== Building Benchmark ==="
cd "$BENCHMARK_DIR"
rm -rf build
mkdir -p build && cd build

cmake -DENABLE_NPU=OFF \
  -DTFLITE_INCLUDE_DIR="${TF_DIR}" \
  -DTFLITE_LIB="${TF_DIR}/tflite_build/libtensorflow-lite.a" \
  -DTFLITE_FLATBUFFERS_DIR="${TF_DIR}/tflite_build/flatbuffers/include" \
  -DTFLITE_ABSEIL_DIR="${TF_DIR}/tflite_build/abseil-cpp" \
  -DTFLITE_BUILD_DIR="${TF_DIR}/tflite_build" ..

make -j$(nproc)

echo "=== Build Complete! ==="
echo "Run with: ./imx93_benchmark <model_path> CPU"