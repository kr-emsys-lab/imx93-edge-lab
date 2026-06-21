cmake_minimum_required(VERSION 3.16)
project(imx93_edge_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_NPU "Enable Ethos-U NPU Delegate" ON)

find_path(TFLITE_INCLUDE_DIR tensorflow/lite/interpreter.h)
find_library(TFLITE_LIB NAMES tensorflowlite tensorflow-lite)

if(NOT TFLITE_INCLUDE_DIR OR NOT TFLITE_LIB)
  message(FATAL_ERROR "TensorFlow Lite not found. Set TFLITE_INCLUDE_DIR and TFLITE_LIB.")
endif()

if(ENABLE_NPU)
  # When using the external delegate pattern, the Ethos-U delegate library is loaded at runtime,
  # so build-time linking to the delegate library is not required.
  message(STATUS "Building with NPU runtime support using the TensorFlow Lite external delegate pattern.")
endif()

add_executable(imx93_benchmark
  src/main.cpp
  src/benchmark.cpp
)

target_include_directories(imx93_benchmark PRIVATE
  ${TFLITE_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${TFLITE_FLATBUFFERS_DIR}
  ${TFLITE_ABSEIL_DIR}
)

target_link_libraries(imx93_benchmark PRIVATE
  ${TFLITE_LIB}
)

if(TFLITE_BUILD_DIR)
  # For native static builds, link all downloaded TFLite dependencies (ruy, xnnpack, etc.)
  file(GLOB_RECURSE TFLITE_DEPS "${TFLITE_BUILD_DIR}/*.a")
  
  target_link_options(imx93_benchmark PRIVATE -Wl,--allow-multiple-definition)
  target_link_libraries(imx93_benchmark PRIVATE -Wl,--start-group ${TFLITE_DEPS} -Wl,--end-group pthread dl m)
endif()

if(ENABLE_NPU)
  target_compile_definitions(imx93_benchmark PRIVATE ENABLE_NPU)
endif()
