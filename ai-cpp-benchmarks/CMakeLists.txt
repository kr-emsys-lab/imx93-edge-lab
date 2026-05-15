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
  find_path(ETHOSU_INCLUDE_DIR ethosu_delegate.h)
  find_library(ETHOSU_DELEGATE_LIB NAMES ethosu_delegate libethosu_delegate libethosu_delegate.so.1)
  if(NOT ETHOSU_INCLUDE_DIR OR NOT ETHOSU_DELEGATE_LIB)
    message(FATAL_ERROR "Ethos-U delegate not found. Disable with -DENABLE_NPU=OFF or set paths.")
  endif()
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
  target_include_directories(imx93_benchmark PRIVATE ${ETHOSU_INCLUDE_DIR})
  target_link_libraries(imx93_benchmark PRIVATE ${ETHOSU_DELEGATE_LIB})
  target_compile_definitions(imx93_benchmark PRIVATE ENABLE_NPU)
endif()
