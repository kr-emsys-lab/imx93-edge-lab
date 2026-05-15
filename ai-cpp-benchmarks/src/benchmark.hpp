#pragma once

#include <string>
#include <vector>

namespace imx93 {

enum class HardwareBackend {
    CPU,
    NPU
};

struct BenchmarkConfig {
    std::string model_name;
    std::string model_path;
    HardwareBackend backend;
    int iterations;
};

struct BenchmarkResult {
    std::string model_name;
    HardwareBackend backend;
    double average_latency_ms;
    double speedup_factor;
};

class BenchmarkRunner {
public:
    BenchmarkResult Run(const BenchmarkConfig& config);
    void PrintResults(const std::vector<BenchmarkResult>& results) const;
};

} // namespace imx93