# Host Cryptographic Pipeline Setup

This guide details the offline steps required on the Linux development host machine (WSL2 / Ubuntu) to secure and prepare target AI model assets before deploying them to the i.MX93 hardware.

> ⚠️ **SECURITY DISCLAIMER & PRODUCTION WARNING**
> The local cryptographic operations outlined below are strictly for **proof-of-concept (PoC) and demonstration purposes**. 
> * **The Risk:** Generating and storing raw asymmetric private keys (`model_private_key.pem`) and symmetric deployment keys (`model_aes.key`) on a local development host exposes critical key material to the host environment's file system and memory space.
> * **Production Implementation:** In a production-grade infrastructure, raw key material must never reside on a developer host. The signing and asset-encryption pipeline must be offloaded to a secure, audited environment such as an enterprise Cloud Key Management Service (e.g., AWS KMS, Azure Key Vault, Google Cloud KMS) or a dedicated Hardware Security Module (HSM) integrated into the corporate CI/CD signing pipeline.

---

## 1. Step-by-Step Generation & Asset Packaging

### Step 1: Key Generation
Generate the asymmetric ECDSA key pair used for model signing, and a symmetric AES key for hardware-targeted block decryption.

```bash
# Generate ECDSA Private Key (prime256v1 curve)
openssl ecparam -name prime256v1 -genkey -noout -out model_private_key.pem

# Extract the Public Key (to be deployed/provisioned on the target target)
openssl ec -in model_private_key.pem -pubout -out model_public_key.pem

# Generate a random 256-bit key for AES-256-CBC encryption
openssl rand -hex 32 > model_aes.key
```

### Step 2: Encrypt the Model (AES-256-CBC)
Encrypt the plaintext `.tflite` with a fresh random IV, then **prepend the raw 16-byte IV** to the ciphertext. The runtime reads the IV from the first 16 bytes of the `.enc` file.

```bash
# Random 16-byte IV (hex)
IV_HEX=$(openssl rand -hex 16)

# AES-256-CBC encryption using the raw key (no salt/KDF, so it matches the runtime)
openssl enc -aes-256-cbc -K "$(cat model_aes.key)" -iv "$IV_HEX" \
    -in model.tflite -out ciphertext.bin

# Final asset = [16 raw IV bytes][ciphertext]
printf '%s' "$IV_HEX" | xxd -r -p > model.tflite.enc
cat ciphertext.bin >> model.tflite.enc
```

### Step 3: Sign the Model (ECDSA / SHA-256)
The signature is computed over the **plaintext** model. The runtime decrypts first, hashes the recovered plaintext with SHA-256, then verifies against this detached signature.

```bash
openssl dgst -sha256 -sign model_private_key.pem -out model.tflite.sig model.tflite
```

### Encrypted File Format
| Offset | Size      | Contents                          |
| :----- | :-------- | :-------------------------------- |
| 0      | 16 bytes  | AES-CBC initialization vector     |
| 16     | remainder | AES-256-CBC ciphertext (PKCS#7)   |

> 💡 **Shortcut:** `scripts/prepare_model.sh <model.tflite> [out_dir]` runs all three steps above (generating keys if absent) and prints the exact `ai-cpp-validation` command to run on the target.