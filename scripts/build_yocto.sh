#!/bin/bash
set -e

# Resolve absolute paths
WORKSPACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCHMARK_DIR="${WORKSPACE_DIR}/ai-cpp-benchmarks"
SDK_DIR="${WORKSPACE_DIR}/yocto_sdk"
SDK_INSTALLER="${WORKSPACE_DIR}/sdk_installer.sh"

# TODO: Replace this URL with your actual SDK hosting location, 
# or manually place your NXP .sh installer at $WORKSPACE_DIR/sdk_installer.sh
SDK_URL="https://example.com/imx93-sdk-installer.sh"

CLEAN_BUILD=0
CLEAN_ALL=0

if [[ "$1" == "clean" ]]; then
    CLEAN_BUILD=1
    if [[ "$2" == "all" ]]; then
        CLEAN_ALL=1
    fi
fi

echo "=== i.MX93 Yocto Cross-Compilation Setup ==="

if [ "$CLEAN_ALL" -eq 1 ]; then
    echo "[Clean All] Removing SDK, Installer, and Build directories..."
    rm -rf "${SDK_DIR}"
    rm -f "${SDK_INSTALLER}"
    rm -rf "${BENCHMARK_DIR}/build_yocto"
elif [ "$CLEAN_BUILD" -eq 1 ]; then
    echo "[Clean] Removing Build directory..."
    rm -rf "${BENCHMARK_DIR}/build_yocto"
fi

# 1. Install Host Prerequisites
echo "Checking prerequisites..."
sudo apt-get update -yqq
sudo apt-get install -yqq wget xz-utils build-essential cmake

# 2. Download and Extract SDK
if [ ! -d "${SDK_DIR}" ]; then
    if [ ! -f "${SDK_INSTALLER}" ]; then
        echo "Downloading Yocto SDK..."
        curl -f -L -o "${SDK_INSTALLER}" "${SDK_URL}" || {
            echo "ERROR: Failed to download SDK. Please update SDK_URL or place the .sh installer manually as sdk_installer.sh in the root."
            exit 1
        }
    fi
    chmod +x "${SDK_INSTALLER}"
    echo "Installing Yocto SDK to ${SDK_DIR}..."
    "${SDK_INSTALLER}" -y -d "${SDK_DIR}"
else
    echo "Yocto SDK already installed at ${SDK_DIR}."
fi

# 3. Source SDK Environment & Build
echo "Sourcing SDK environment and building..."
source "${SDK_DIR}"/environment-setup-aarch64-*

cd "${BENCHMARK_DIR}"
mkdir -p build_yocto && cd build_yocto
cmake -DENABLE_NPU=ON ..
make -j$(nproc)

echo "=== Yocto Build Complete! ==="
echo "Executable is located at: ${BENCHMARK_DIR}/build_yocto/imx93_benchmark"