# HydraFlow-X Ultra-Fast Trading Components
cmake_minimum_required(VERSION 3.20)

# Ultra-fast trading library
add_library(hfx-ultra STATIC
    src/ultra_fast_mempool.cpp
    src/v3_tick_engine.cpp
    src/jito_mev_engine.cpp
    src/smart_trading_engine.cpp
    src/mev_shield.cpp
    src/hsm_key_manager.cpp
    # Temporarily excluded: complex infrastructure components
    src/nats_jetstream_engine.cpp
    src/monitoring_system.cpp
    src/security_manager.cpp
    src/testing_framework.cpp
    # src/production_database.cpp
    # src/system_testing.cpp
)

target_include_directories(hfx-ultra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
)

# High-performance compilation flags
target_compile_options(hfx-ultra PRIVATE
    -O3
    -march=native
    -mtune=native
    -flto
    -ffast-math
    -funroll-loops
    -finline-functions
    -fomit-frame-pointer
    -DNDEBUG
)

# Ultra-low latency specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(hfx-ultra PRIVATE
        -march=native
        -mtune=native
        -mavx2                  # AVX2 instructions for SIMD
        -mfma                   # Fused multiply-add
        -msse4.2               # SSE 4.2
        -mpopcnt               # Population count instruction
        -mbmi                  # Bit manipulation instructions
        -mbmi2                 # Bit manipulation instructions 2
        -mlzcnt                # Leading zero count
        -mprfchw               # Prefetch write
        -madx                  # Multi-precision add-carry
        -mrdrnd                # Random number generation
        -mf16c                 # 16-bit floating point conversion
        -mxsave                # Extended state save/restore
        -mxsaveopt             # Extended state save/restore optimization
    )
endif()

# Apple-specific optimizations
if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        # Apple Silicon optimizations
        target_compile_options(hfx-ultra PRIVATE
            -mcpu=apple-a14        # Apple A14 or later
            -mtune=apple-a14
        )
    else()
        # Intel Mac optimizations
        target_compile_options(hfx-ultra PRIVATE
            -march=skylake
            -mtune=skylake
        )
    endif()
endif()

# Find required libraries
find_package(OpenSSL REQUIRED)

# Optional: Find NATS C client (can be installed via package manager or built from source)
# For now, we'll use a mock implementation that can be switched to real NATS later
option(USE_REAL_NATS "Use real NATS C client library" OFF)
if(USE_REAL_NATS)
    find_library(NATS_LIB nats)
    if(NATS_LIB)
        target_compile_definitions(hfx-ultra PRIVATE USE_REAL_NATS=1)
        target_link_libraries(hfx-ultra PRIVATE ${NATS_LIB})
    endif()
endif()

# Optional: Database drivers
option(USE_POSTGRESQL "Use PostgreSQL database driver" OFF)
option(USE_CLICKHOUSE "Use ClickHouse database driver" OFF)

if(USE_POSTGRESQL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBPQ REQUIRED libpq)
    target_compile_definitions(hfx-ultra PRIVATE USE_POSTGRESQL=1)
    target_link_libraries(hfx-ultra PRIVATE ${LIBPQ_LIBRARIES})
    target_include_directories(hfx-ultra PRIVATE ${LIBPQ_INCLUDE_DIRS})
endif()

if(USE_CLICKHOUSE)
    target_compile_definitions(hfx-ultra PRIVATE USE_CLICKHOUSE=1)
    # ClickHouse C++ client would be linked here
endif()

# Link system libraries for threading and high-resolution timing
if(UNIX AND NOT APPLE)
    target_link_libraries(hfx-ultra PRIVATE pthread rt OpenSSL::SSL OpenSSL::Crypto)
elseif(APPLE)
    target_link_libraries(hfx-ultra PRIVATE pthread OpenSSL::SSL OpenSSL::Crypto)
    # Add Security framework for macOS HSM integration
    find_library(SECURITY_FRAMEWORK Security)
    if(SECURITY_FRAMEWORK)
        target_link_libraries(hfx-ultra PRIVATE ${SECURITY_FRAMEWORK})
    endif()
endif()

# Enable link-time optimization for maximum performance
set_property(TARGET hfx-ultra PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

# Memory alignment and cache optimization
target_compile_definitions(hfx-ultra PRIVATE
    HFX_CACHE_LINE_SIZE=64
    HFX_ENABLE_SIMD=1
    HFX_ULTRA_FAST_MODE=1
)

# Platform-specific networking optimizations
if(UNIX)
    target_compile_definitions(hfx-ultra PRIVATE
        HFX_ENABLE_EPOLL=1
        HFX_ENABLE_SPLICE=1
        HFX_ENABLE_SENDFILE=1
    )
endif()

if(APPLE)
    target_compile_definitions(hfx-ultra PRIVATE
        HFX_ENABLE_KQUEUE=1
    )
endif()

# Install headers
install(DIRECTORY include/
        DESTINATION include/hfx-ultra
        FILES_MATCHING PATTERN "*.hpp")

# Install library
install(TARGETS hfx-ultra
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
