cmake_minimum_required(VERSION 3.20)
project(hfx-security VERSION 1.0.0 LANGUAGES CXX)

# Find required packages
find_package(OpenSSL REQUIRED)

# Create static library
add_library(hfx-security STATIC
    src/security_middleware.cpp
    src/rate_limiter.cpp
    src/ddos_protection.cpp
    src/api_security.cpp
)

# Include directories
target_include_directories(hfx-security PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
    ${CMAKE_SOURCE_DIR}/hfx-log/include
)

# Link dependencies
target_link_libraries(hfx-security PRIVATE
    hfx-core
    hfx-log
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Compile definitions
target_compile_definitions(hfx-security PRIVATE
    HAS_OPENSSL=1
)

# Set C++ standard
target_compile_features(hfx-security PUBLIC cxx_std_23)
