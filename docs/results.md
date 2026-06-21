# i.MX93 Benchmark Results

These measurements were captured directly on the NXP i.MX93 FRDM board, comparing the TensorFlow Lite inference latency of the ARM Cortex-A55 CPU against the Ethos-U65 NPU.

## MobileNet V1 (Quantized)

The following table demonstrates the performance baseline when running a standard `mobilenet_v1_1.0_224_quant.tflite` model (224x224 input resolution). The CPU backend runs the plain quantized model, while the NPU backend runs the Vela-compiled `mobilenet_v1_1.0_224_quant_vela.tflite`, which is required to target the Ethos-U65.

```text
+-----------------------------------------+--------------+--------------------+----------------+
| Model Name                              | Backend      | Avg Latency (ms)   | Speedup Factor |
+-----------------------------------------+--------------+--------------------+----------------+
| mobilenet_v1_1.0_224_quant.tflite       | CPU          | 51.23              | 1.00x          |
| mobilenet_v1_1.0_224_quant_vela.tflite  | NPU          | 3.69               | 13.88x         |
+-----------------------------------------+--------------+--------------------+----------------+
```

> Measured on the NXP i.MX93 FRDM board (Cortex-A55 CPU vs. Ethos-U65 NPU) using TensorFlow Lite 2.19.0, averaged over 100 timed inferences. The NPU delegate reports `1 nodes delegated out of 1 nodes`, confirming the full graph runs on the Ethos-U65.

👉 **[Back to Home](index.md)**