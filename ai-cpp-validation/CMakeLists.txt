cmake_minimum_required(VERSION 3.16)
project(ai-cpp-validation-security CXX)

# 1. Standard C++ setup
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Find OpenSSL (libcrypto is mandatory for the software engine)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

# 3. Optional NXP SMW PKCS#11 backend for EdgeLock Enclave access.
option(ENABLE_SMW_PKCS11 "Enable the NXP SMW PKCS#11 engine" OFF)

# 4. Target executable
add_executable(ai-cpp-validation
    src/main.cpp
    src/validation.cpp
    src/openssl_engine.cpp
)
target_include_directories(ai-cpp-validation PRIVATE src)

# 5. Link libraries
target_link_libraries(ai-cpp-validation PRIVATE OpenSSL::Crypto)

if(ENABLE_SMW_PKCS11)
    find_path(SMW_PKCS11_INCLUDE_DIR
        NAMES pkcs11.h
        PATH_SUFFIXES smw_pkcs11
    )
    if(NOT SMW_PKCS11_INCLUDE_DIR)
        message(FATAL_ERROR "SMW PKCS#11 headers not found; set SMW_PKCS11_INCLUDE_DIR")
    endif()
    target_sources(ai-cpp-validation PRIVATE src/pkcs11_engine.cpp)
    target_include_directories(ai-cpp-validation PRIVATE ${SMW_PKCS11_INCLUDE_DIR})
    target_compile_definitions(ai-cpp-validation PRIVATE HAS_SMW_PKCS11=1)
    target_link_libraries(ai-cpp-validation PRIVATE ${CMAKE_DL_LIBS})
endif()
