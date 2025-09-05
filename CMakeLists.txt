# HydraFlow-X Ultra-Low Latency Trading Platform
# Root CMake Configuration

cmake_minimum_required(VERSION 3.20)

# Set C++ standard before project declaration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(HydraFlow-X
    VERSION 1.0.0
    DESCRIPTION "Ultra-Low Latency DeFi Trading Platform"
    LANGUAGES CXX C
)

# Global configuration options
option(BUILD_WEB_DASHBOARD "Build the web dashboard" ON)
option(BUILD_CORE_BACKEND "Build the core backend" ON)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(ENABLE_PROFILING "Enable profiling" OFF)

# Global compiler flags and optimizations
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

# Platform-specific optimizations
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(HFX_APPLE_SILICON ON)
    message(STATUS "🎯 Apple Silicon (ARM64) detected")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(HFX_INTEL_X86_64 ON)
    message(STATUS "🎯 Intel x86_64 detected")
endif()

# Global compile definitions
add_compile_definitions(
    HFX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    HFX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    HFX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Include directories for cross-module dependencies
include_directories(
    ${CMAKE_SOURCE_DIR}/core-backend
    ${CMAKE_SOURCE_DIR}/core-backend/hfx-core/include
    ${CMAKE_SOURCE_DIR}/core-backend/hfx-net/include
    ${CMAKE_SOURCE_DIR}/core-backend/hfx-log/include
    ${CMAKE_SOURCE_DIR}/core-backend/hfx-ultra/include
    ${CMAKE_SOURCE_DIR}/core-backend/hfx-ai/include
    ${CMAKE_SOURCE_DIR}/core-backend/hfx-api/include
    # Root-level modules
    ${CMAKE_SOURCE_DIR}/hfx-chain/include
    ${CMAKE_SOURCE_DIR}/hfx-strat/include
    ${CMAKE_SOURCE_DIR}/hfx-risk/include
    ${CMAKE_SOURCE_DIR}/hfx-hedge/include
    ${CMAKE_SOURCE_DIR}/hfx-hft/include
    ${CMAKE_SOURCE_DIR}/hfx-mempool/include
    ${CMAKE_SOURCE_DIR}/hfx-mm/include
    ${CMAKE_SOURCE_DIR}/hfx-viz/include
)

# Core backend build
if(BUILD_CORE_BACKEND)
    message(STATUS "🔧 Building core backend")

    # Add all trading modules
    add_subdirectory(hfx-db)
    # add_subdirectory(hfx-chain)  # Temporarily disabled to stop noisy build logs; re-enable after RPC creds
    add_subdirectory(hfx-strat)
    add_subdirectory(hfx-risk)
    add_subdirectory(hfx-auth)
    # add_subdirectory(hfx-monitor)  # Temporarily disabled due to compilation issues
    add_subdirectory(hfx-security)
    add_subdirectory(hfx-backtest)
    add_subdirectory(hfx-sentiment)
    add_subdirectory(hfx-mempool)
    add_subdirectory(hfx-hedge)
    add_subdirectory(hfx-hft)
    add_subdirectory(hfx-mm)
    add_subdirectory(hfx-viz)
    # add_subdirectory(tests)  # Temporarily disabled for build testing

    # Core backend (depends on above modules)
    add_subdirectory(core-backend)
endif()

# Installation configuration
if(BUILD_CORE_BACKEND)
    install(TARGETS hydraflow-x
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    # Install configuration files
    install(DIRECTORY core-backend/config/
        DESTINATION etc/hydraflow-x
        FILES_MATCHING PATTERN "*.json" PATTERN "*.env"
    )
    
    # Install scripts
    install(DIRECTORY scripts/
        DESTINATION bin
        FILES_MATCHING PATTERN "*.sh" PATTERN "*.ps1"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                   GROUP_READ GROUP_EXECUTE
                   WORLD_READ WORLD_EXECUTE
    )
endif()

# Web dashboard (Node.js/Next.js - managed separately)
if(BUILD_WEB_DASHBOARD)
    message(STATUS "🌐 Web dashboard available in ./web-dashboard")
    message(STATUS "   Run: cd web-dashboard && npm install && npm run dev")
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Package configuration
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_NAME "HydraFlow-X")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ultra-Low Latency DeFi Trading Platform")
set(CPACK_PACKAGE_VENDOR "HydraFlow-X Team")

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "🚀 HydraFlow-X Configuration Summary")
message(STATUS "====================================")
message(STATUS "• Version: ${PROJECT_VERSION}")
message(STATUS "• Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "• C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "• Core Backend: ${BUILD_CORE_BACKEND}")
message(STATUS "• Web Dashboard: ${BUILD_WEB_DASHBOARD}")
message(STATUS "• Tests: ${BUILD_TESTS}")
message(STATUS "• Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "• Profiling: ${ENABLE_PROFILING}")
message(STATUS "====================================")
message(STATUS "")
