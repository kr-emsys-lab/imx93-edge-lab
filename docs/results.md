# i.MX93 Hardware Benchmark Results

These measurements were captured directly on the NXP i.MX93 Evaluation Kit, comparing the TensorFlow Lite inference latency of the ARM Cortex-A55 CPU against the Ethos-U65 NPU.

## MobileNet V2 (Quantized)

The following table demonstrates the performance baseline when running a standard `mobilenet_v2_1.0_224_quant.tflite` model (224x224 input resolution).

```text
+-----------------------------------+--------------+--------------------+----------------+
| Model Name                        | Backend      | Avg Latency (ms)   | Speedup Factor |
+-----------------------------------+--------------+--------------------+----------------+
| mobilenet_v2_1.0_224_quant.tflite | CPU          | 14.40              | 1.00x          |
| mobilenet_v2_1.0_224_quant.tflite | NPU          | TBD                | TBD            |
+-----------------------------------+--------------+--------------------+----------------+
```

← Back to Home