# imx93-edge-lab Technical Specification

## Purpose
Provide a developer-facing technical specification for the i.MX93 performance benchmark tool, comparing CPU vs NPU inference latency using TensorFlow Lite and Ethos-U65.

## Scope
- Target platform: NXP i.MX93 (AArch64)
- CPU: Cortex-A55
- NPU: Ethos-U65 with `ethosu_delegate`
- AI framework: TensorFlow Lite C++ API
- Input: static image buffer only
- Measurement: only `interpreter->Invoke()` using `std::chrono::high_resolution_clock`
- Output: formatted CLI table with average latency and speedup factor

## Requirements
1. Use TensorFlow Lite C++ API with `tflite::Interpreter`.
2. Support two backends:
   - CPU builtin interpreter
   - NPU via `ethosu_delegate`
3. Load Vela-compiled `.tflite` models for Ethos-U65.
4. Prepare a static input buffer matching the model input tensor.
5. Warm-up before measurement, then run 100 timed inferences.
6. Measure only the `Invoke()` call.
7. Print a CLI table with:
   - Model Name
   - Hardware Backend
   - Average Latency (ms)
   - Speedup Factor

## Architecture

### Components
- `BenchmarkConfig`
  - `model_name`
  - `model_path`
  - `backend`
  - `iterations`
  - static input buffer
- `BenchmarkResult`
  - `model_name`
  - `backend`
  - `average_latency_ms`
  - `speedup_factor`
- `StaticImageInput`
  - load fixed image data or create deterministic synthetic data
- `ModelInterpreter`
  - load model
  - initialize interpreter
  - apply Ethos-U delegate for NPU
  - allocate tensors
  - set input buffer
  - invoke inference
- `LatencyBenchmark`
  - warm-up runs
  - timed runs
  - average latency computation
- `CliReporter`
  - tabular output
  - speedup factor calculation

## Benchmark Output
Example table format:

```
+----------------+--------------+--------------------+----------------+
| Model Name     | Backend      | Avg Latency (ms)   | Speedup Factor |
+----------------+--------------+--------------------+----------------+
| mobilenet_v2   | CPU          | 92.5               | 1.00x          |
| mobilenet_v2   | NPU          | 12.4               | 7.46x          |
+----------------+--------------+--------------------+----------------+
```

## Implementation Roadmap

1. Project scaffolding
   - create `src/`, `benchmarks/`, `models/`, `platform/`, `scripts/`, `tests/`
   - add `SPEC.md` and `Manifest.json`
2. CMake configuration
   - require C++17
   - locate `tensorflow-lite` and `ethosu_delegate`
   - link against `libtensorflow-lite` and `libethosu_delegate`
3. Define interfaces in header(s)
   - `BenchmarkConfig`, `BenchmarkResult`, `HardwareBackend`
   - `StaticImageInput`, `ModelInterpreter`, `BenchmarkRunner`
4. Implement static input buffer
   - support file load or synthetic image creation
   - validate tensor shape
5. Implement model/interpreter initialization
   - load model with `tflite::FlatBufferModel`
   - build interpreter with `InterpreterBuilder`
   - allocate tensors
   - attach delegate for NPU backend
6. Implement latency measurement
   - do warm-up invokes first
   - time 100 `Invoke()` calls
   - compute average latency in milliseconds
7. Implement result generation
   - collect benchmark results per backend
   - compute NPU speedup relative to CPU
8. Implement CLI reporter
   - print structured table
   - allow user to select model and backend via arguments
9. Validate and test
   - run benchmark on i.MX93 hardware
   - verify correct delegate use and output shapes
   - compare CPU vs NPU latency
10. Document
   - add developer notes in `docs/`
   - add usage examples, model expectations, and build instructions

## CMake Requirements
- `cmake_minimum_required(VERSION 3.16)`
- `set(CMAKE_CXX_STANDARD 17)`
- `find_path(TFLITE_INCLUDE_DIR tensorflow/lite/interpreter.h)`
- `find_library(TFLITE_LIB NAMES tensorflowlite tensorflow-lite)`
- `find_path(ETHOSU_INCLUDE_DIR ethosu_delegate.h)`
- `find_library(ETHOSU_DELEGATE_LIB NAMES ethosu_delegate libethosu_delegate)`
- `target_include_directories(... PRIVATE ${TFLITE_INCLUDE_DIR} ${ETHOSU_INCLUDE_DIR})`
- `target_link_libraries(... PRIVATE ${TFLITE_LIB} ${ETHOSU_DELEGATE_LIB})`
