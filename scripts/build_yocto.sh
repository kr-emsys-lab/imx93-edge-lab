#!/bin/bash
set -e

# Resolve absolute paths
WORKSPACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCHMARK_DIR="${WORKSPACE_DIR}/ai-cpp-benchmarks"
YOCTO_DIR="${WORKSPACE_DIR}/yocto_build"
SDK_DIR="${WORKSPACE_DIR}/yocto_sdk"

CLEAN_BUILD=0
CLEAN_ALL=0

if [[ "$1" == "clean" ]]; then
    CLEAN_BUILD=1
    if [[ "$2" == "all" ]]; then
        CLEAN_ALL=1
    fi
fi

echo "=== i.MX93 Yocto Project & SDK Build Setup ==="

if [ "$CLEAN_ALL" -eq 1 ]; then
    echo "[Clean All] Wiping out Yocto repository, SDK, and CMake build..."
    rm -rf "${YOCTO_DIR}"
    rm -rf "${SDK_DIR}"
    rm -rf "${BENCHMARK_DIR}/build_yocto"
    exit 0
elif [ "$CLEAN_BUILD" -eq 1 ]; then
    echo "[Clean] Removing Benchmark build and Yocto tmp artifacts (keeping cache)..."
    rm -rf "${BENCHMARK_DIR}/build_yocto"
    rm -rf "${YOCTO_DIR}/build/tmp"
    exit 0
fi

echo "WARNING: Building Yocto from scratch requires ~100GB+ of free space and several hours."
echo "Ensure your PC does not sleep during this process."
sleep 3

# 1. Install Yocto Host Prerequisites
echo "Installing host prerequisites for Yocto..."
sudo apt-get update
sudo apt-get install -y build-essential chrpath cpio debianutils diffstat file gawk \
gcc git iputils-ping libacl1 liblz4-tool locales python3 python3-git \
python3-jinja2 python3-pexpect python3-pip python3-subunit socat texinfo \
unzip wget xz-utils zstd efitools

# Ensure the required locale is available
echo "Generating en_US.UTF-8 locale..."
sudo locale-gen en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8

# Ensure 'repo' tool is installed
if ! command -v repo &> /dev/null; then
    echo "Installing 'repo' tool..."
    mkdir -p ~/.bin
    curl -s -o ~/.bin/repo https://storage.googleapis.com/git-repo-downloads/repo
    chmod a+x ~/.bin/repo
fi
export PATH="$HOME/.bin:$PATH"

# Git config check (repo requires this)
if ! git config --global user.name &> /dev/null; then
    echo "ERROR: Please configure git user.name: git config --global user.name 'Your Name'"
    exit 1
fi
if ! git config --global user.email &> /dev/null; then
    echo "ERROR: Please configure git user.email: git config --global user.email 'you@example.com'"
    exit 1
fi

# 2. Sync NXP Yocto BSP
mkdir -p "${YOCTO_DIR}"
cd "${YOCTO_DIR}"

if [ ! -d ".repo" ]; then
    echo "Initializing NXP i.MX Yocto Manifest (imx-6.6.23-2.0.0 Scarthgap)..."
    repo init -u https://github.com/nxp-imx/imx-manifest -b imx-linux-scarthgap -m imx-6.6.23-2.0.0.xml
fi

echo "Syncing Yocto repositories (this might take a while)..."
repo sync -j$(nproc)

# 3. Bitbake the SDK
echo "Setting up build environment..."
# Use frdm-imx93 as default, override by exporting MACHINE=imx93-11x11-lpddr4x-evk if needed
export MACHINE=${MACHINE:-frdm-imx93}
export DISTRO=fsl-imx-xwayland
export EULA=1

source imx-setup-release.sh -b build

if [ ! -d "${SDK_DIR}" ]; then
    echo "Bitbaking the SDK..."
    bitbake imx-image-full -c populate_sdk

    # 4. Install the newly built SDK
    SDK_INSTALLER=$(find tmp/deploy/sdk/ -name "fsl-imx-xwayland-glibc-x86_64-imx-image-full-*.sh" | head -n 1)
    if [ -z "$SDK_INSTALLER" ]; then
        echo "ERROR: SDK installer not found after build!"
        exit 1
    fi

    echo "Installing Yocto SDK to ${SDK_DIR}..."
    chmod +x "${SDK_INSTALLER}"
    "${SDK_INSTALLER}" -y -d "${SDK_DIR}"
else
    echo "Yocto SDK already installed at ${SDK_DIR}."
fi

# 5. Build the Benchmark using the new SDK
echo "Sourcing SDK environment and building the benchmark..."
source "${SDK_DIR}"/environment-setup-aarch64-*

cd "${BENCHMARK_DIR}"
mkdir -p build_yocto && cd build_yocto
cmake -DENABLE_NPU=ON ..
make -j$(nproc)

echo "=== Yocto Full Build Complete! ==="
echo "Executable is located at: ${BENCHMARK_DIR}/build_yocto/imx93_benchmark"