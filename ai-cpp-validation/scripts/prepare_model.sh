#!/bin/bash
#
# Offline host tooling for the ai-cpp-validation OpenSSL path.
#
# Generates the cryptographic assets and produces an encrypted model plus a
# detached signature in the exact format the C++ runtime expects:
#   - AES key:   64 hex chars (32 bytes) -> model_aes.key
#   - ECDSA key: prime256v1 -> model_private_key.pem / model_public_key.pem
#   - .enc file: [16 raw IV bytes][AES-256-CBC ciphertext]
#   - .sig file: ECDSA-SHA256 signature over the PLAINTEXT model
#
# Usage:
#   prepare_model.sh <plaintext_model> [output_dir] [keys_dir]
#
# WARNING: For proof-of-concept / local validation only. Raw key material on a
# developer host is insecure; production signing must use an HSM/KMS. See
# host-crypto-setup.md.

set -euo pipefail

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <plaintext_model> [output_dir] [keys_dir]" >&2
    exit 1
fi

PLAIN="$1"
OUT_DIR="${2:-.}"
KEYS_DIR="${3:-$OUT_DIR}"

if [ ! -f "$PLAIN" ]; then
    echo "Error: plaintext model not found: $PLAIN" >&2
    exit 1
fi

mkdir -p "$OUT_DIR" "$KEYS_DIR"

AES_KEY="$KEYS_DIR/model_aes.key"
PRIV_KEY="$KEYS_DIR/model_private_key.pem"
PUB_KEY="$KEYS_DIR/model_public_key.pem"

BASENAME="$(basename "$PLAIN")"
ENC="$OUT_DIR/${BASENAME}.enc"
SIG="$OUT_DIR/${BASENAME}.sig"

# 1. Keys (reused if present so repeated runs stay deterministic).
if [ ! -f "$AES_KEY" ]; then
    echo "Generating AES-256 key -> $AES_KEY"
    openssl rand -hex 32 > "$AES_KEY"
fi
if [ ! -f "$PRIV_KEY" ]; then
    echo "Generating ECDSA P-256 key pair -> $PRIV_KEY / $PUB_KEY"
    openssl ecparam -name prime256v1 -genkey -noout -out "$PRIV_KEY"
    openssl ec -in "$PRIV_KEY" -pubout -out "$PUB_KEY"
fi

# 2. Encrypt: random IV, AES-256-CBC, then prepend the raw IV to the ciphertext.
IV_HEX="$(openssl rand -hex 16)"
TMP_CT="$(mktemp)"
trap 'rm -f "$TMP_CT"' EXIT

openssl enc -aes-256-cbc \
    -K "$(cat "$AES_KEY")" \
    -iv "$IV_HEX" \
    -in "$PLAIN" \
    -out "$TMP_CT"

printf '%s' "$IV_HEX" | xxd -r -p > "$ENC"
cat "$TMP_CT" >> "$ENC"
echo "Encrypted model -> $ENC"

# 3. Sign the PLAINTEXT model (ECDSA over SHA-256).
openssl dgst -sha256 -sign "$PRIV_KEY" -out "$SIG" "$PLAIN"
echo "Signature -> $SIG"

echo ""
echo "Done. Verify on target with:"
echo "  ai-cpp-validation --engine openssl \\"
echo "    --model $ENC \\"
echo "    --signature $SIG \\"
echo "    --aes-key $AES_KEY \\"
echo "    --public-key $PUB_KEY"
