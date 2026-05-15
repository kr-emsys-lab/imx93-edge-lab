#!/bin/bash
set -e

MODELS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../ai-cpp-benchmarks" && pwd)/models"
mkdir -p "$MODELS_DIR"

echo "Downloading MobileNet V2 (CPU)..."
curl -L -o "$MODELS_DIR/mobilenet_v2_1.0_224_quant.tflite" "https://github.com/google-coral/test_data/raw/master/mobilenet_v2_1.0_224_quant.tflite"

echo "Done! Models saved to $MODELS_DIR"