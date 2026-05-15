#include "benchmark.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <memory>
#include <cstring>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#ifdef ENABLE_NPU
#include "ethosu_delegate.h"
#endif

namespace imx93 {

namespace {
void PopulateStaticInput(TfLiteTensor* tensor) {
    // For pure latency benchmarks, zeroing out the input tensor is sufficient 
    // to simulate a static image buffer of the correct shape.
    if (tensor->bytes > 0 && tensor->data.raw != nullptr) {
        std::memset(tensor->data.raw, 0x01, tensor->bytes); // Fill with deterministic dummy data
    }
}
} // namespace

BenchmarkResult BenchmarkRunner::Run(const BenchmarkConfig& config) {
    BenchmarkResult result;
    result.model_name = config.model_name;
    result.backend = config.backend;
    result.average_latency_ms = 0.0;
    result.speedup_factor = 1.0;

    // Load Model
    auto model = tflite::FlatBufferModel::BuildFromFile(config.model_path.c_str());
    if (!model) {
        std::cerr << "Failed to mmap model: " << config.model_path << "\n";
        return result;
    }

    // Build Interpreter
    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;
    tflite::InterpreterBuilder(*model, resolver)(&interpreter);
    if (!interpreter) {
        std::cerr << "Failed to construct interpreter\n";
        return result;
    }

    // Apply Delegate if NPU
#ifdef ENABLE_NPU
    TfLiteDelegate* ethosu_delegate = nullptr;
    if (config.backend == HardwareBackend::NPU) {
        ethosu_delegate_options options = ethosu_delegate_options_default();
        ethosu_delegate = ethosu_delegate_create(&options);
        if (interpreter->ModifyGraphWithDelegate(ethosu_delegate) != kTfLiteOk) {
            std::cerr << "Failed to apply Ethos-U delegate\n";
        }
    }
#else
    if (config.backend == HardwareBackend::NPU) {
        std::cerr << "Warning: Compiled without NPU support. Please run with CPU backend.\n";
        return result;
    }
#endif

    // Allocate Tensors
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        std::cerr << "Failed to allocate tensors\n";
#ifdef ENABLE_NPU
        if (ethosu_delegate) ethosu_delegate_delete(ethosu_delegate);
#endif
        return result;
    }

    // Prepare Static Input
    int input_idx = interpreter->inputs()[0];
    TfLiteTensor* input_tensor = interpreter->tensor(input_idx);
    PopulateStaticInput(input_tensor);

    // Warm-up runs
    const int warmup_runs = 5;
    for (int i = 0; i < warmup_runs; ++i) {
        interpreter->Invoke();
    }

    // Timed runs
    double total_time_ms = 0.0;
    for (int i = 0; i < config.iterations; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (interpreter->Invoke() != kTfLiteOk) {
            std::cerr << "Failed to invoke interpreter\n";
            break;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end_time - start_time;
        total_time_ms += elapsed.count();
    }

    result.average_latency_ms = total_time_ms / config.iterations;

    // Cleanup
#ifdef ENABLE_NPU
    if (ethosu_delegate) {
        ethosu_delegate_delete(ethosu_delegate);
    }
#endif

    return result;
}

void BenchmarkRunner::PrintResults(const std::vector<BenchmarkResult>& results) const {
    std::cout << "\n+" << std::string(16, '-') << "+" << std::string(14, '-') 
              << "+" << std::string(20, '-') << "+" << std::string(16, '-') << "+\n";
    std::cout << "| " << std::left << std::setw(14) << "Model Name"
              << " | " << std::setw(12) << "Backend"
              << " | " << std::setw(18) << "Avg Latency (ms)"
              << " | " << std::setw(14) << "Speedup Factor" << " |\n";
    std::cout << "+" << std::string(16, '-') << "+" << std::string(14, '-') 
              << "+" << std::string(20, '-') << "+" << std::string(16, '-') << "+\n";

    double cpu_latency = -1.0;

    for (auto result : results) {
        if (result.backend == HardwareBackend::CPU) {
            cpu_latency = result.average_latency_ms;
        } else if (result.backend == HardwareBackend::NPU && cpu_latency > 0) {
            result.speedup_factor = cpu_latency / result.average_latency_ms;
        }

        std::string backend_str = (result.backend == HardwareBackend::CPU) ? "CPU" : "NPU";
        
        std::cout << "| " << std::left << std::setw(14) << result.model_name
                  << " | " << std::setw(12) << backend_str
                  << " | " << std::setw(18) << std::fixed << std::setprecision(2) << result.average_latency_ms
                  << " | " << std::setw(13) << std::fixed << std::setprecision(2) << result.speedup_factor << "x |\n";
    }

    std::cout << "+" << std::string(16, '-') << "+" << std::string(14, '-') 
              << "+" << std::string(20, '-') << "+" << std::string(16, '-') << "+\n";
}

} // namespace imx93