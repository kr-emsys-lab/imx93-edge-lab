#include "benchmark.hpp"
#include <iostream>
#include <string>
#include <vector>

void PrintUsage(const char* program_name) {
  std::cerr << "Usage: " << program_name << " <model_path> [backend: CPU|NPU|ALL]\n";
}

int main(int argc, char** argv) {
  if (argc < 2) {
    PrintUsage(argv[0]);
    return 1;
  }

  std::string model_path = argv[1];
  std::string backend_arg = (argc > 2) ? argv[2] : "ALL";

  // Extract a clean model name from the file path for reporting
  std::string model_name = model_path;
  size_t last_slash = model_path.find_last_of("/\\");
  if (last_slash != std::string::npos) {
    model_name = model_path.substr(last_slash + 1);
  }

  imx93::BenchmarkRunner runner;
  std::vector<imx93::BenchmarkResult> results;

  auto run_inference = [&](imx93::HardwareBackend backend) {
    imx93::BenchmarkConfig config;
    config.model_name = model_name;
    config.model_path = model_path;
    config.backend = backend;
    config.iterations = 100; // As required by SPEC.md

    std::cout << "Running benchmark for " << model_name 
              << " on " << (backend == imx93::HardwareBackend::CPU ? "CPU" : "NPU") << "...\n";

    results.push_back(runner.Run(config));
  };

  if (backend_arg == "CPU" || backend_arg == "ALL") {
    run_inference(imx93::HardwareBackend::CPU);
  }
  
  if (backend_arg == "NPU" || backend_arg == "ALL") {
    run_inference(imx93::HardwareBackend::NPU);
  }

  runner.PrintResults(results);

  return 0;
}
