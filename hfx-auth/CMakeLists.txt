cmake_minimum_required(VERSION 3.20)
project(hfx-auth VERSION 1.0.0 LANGUAGES CXX)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Create static library
add_library(hfx-auth STATIC
    src/auth_manager.cpp
    src/jwt_manager.cpp
    src/stubs.cpp
    src/api_key_manager.cpp
)

# Include directories
target_include_directories(hfx-auth PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
    ${CMAKE_SOURCE_DIR}/hfx-log/include
    ${CMAKE_SOURCE_DIR}/hfx-db/include
)

# Link dependencies
target_link_libraries(hfx-auth PRIVATE
    hfx-core
    hfx-log
    hfx-db
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Compile definitions
target_compile_definitions(hfx-auth PRIVATE
    HAS_OPENSSL=1
)

# Set C++ standard
target_compile_features(hfx-auth PUBLIC cxx_std_23)
