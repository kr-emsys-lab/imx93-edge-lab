cmake_minimum_required(VERSION 3.16)
project(ai-cpp-validation-security CXX)

# 1. Standard C++ setup
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Find OpenSSL (libcrypto is mandatory for the software engine)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

# 3. Optional/conditional target compilation for the NXP EdgeLock Enclave.
#    The ELE engine itself lands in Phase 3.2; this wiring is kept ready.
option(ENABLE_IMX_ELE "Enable NXP EdgeLock Enclave hardware support" OFF)

# 4. Target executable
add_executable(ai-cpp-validation
    src/main.cpp
    src/validation.cpp
    src/openssl_engine.cpp
)
target_include_directories(ai-cpp-validation PRIVATE src)

# 5. Link libraries
target_link_libraries(ai-cpp-validation PRIVATE OpenSSL::Crypto)

if(ENABLE_IMX_ELE)
    # Expecting the Yocto SDK/sysroot to provide the imx-secure-enclave headers/libs.
    find_library(IMX_HSM_LIB NAMES hsm REQUIRED)
    target_include_directories(ai-cpp-validation PRIVATE /usr/include/imx-secure-enclave)
    target_compile_definitions(ai-cpp-validation PRIVATE HAS_EDGE_LOCK_ENCLAVE=1)
    target_link_libraries(ai-cpp-validation PRIVATE ${IMX_HSM_LIB})
endif()
