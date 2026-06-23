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