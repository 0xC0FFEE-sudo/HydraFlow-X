# HydraFlow-X Core Backend
# Ultra-Low Latency DeFi HFT Engine Implementation

# CMake minimum version and project setup
cmake_minimum_required(VERSION 3.20)
project(HydraFlow-X VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "🚀 HydraFlow-X Build Configuration")
message(STATUS "==================================")

# Architecture detection and optimization
if(NOT DEFINED HFX_TARGET_ARCH)
    if(CMAKE_OSX_ARCHITECTURES)
        set(HFX_TARGET_ARCH ${CMAKE_OSX_ARCHITECTURES})
    else()
        set(HFX_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif()
endif()

message(STATUS "🎯 HydraFlow-X Target Architecture: ${HFX_TARGET_ARCH}")
message(STATUS "🔧 Build Type: ${CMAKE_BUILD_TYPE}")

# Enable Link-Time Optimization (LTO) for release builds
if(CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "🚀 Link-Time Optimization enabled")
endif()

# Ultra-aggressive optimization flags for HFT
if(APPLE)
    # Base optimization flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -finline-functions -funroll-loops")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    
    # Architecture-specific optimizations
    if(HFX_TARGET_ARCH MATCHES "arm64" OR CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        message(STATUS "🍎 Applying Apple Silicon (ARM64) optimizations")
        
        # Apple Silicon specific optimizations  
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=apple-m1 -mtune=apple-m1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -falign-functions=32 -falign-loops=32")
        
        # ARM64 memory optimizations
        add_compile_definitions(HFX_APPLE_SILICON=1)
        add_compile_definitions(HFX_ARM64_NEON=1)
        
    elseif(HFX_TARGET_ARCH MATCHES "x86_64" OR CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        message(STATUS "⚡ Applying Intel x86_64 optimizations")
        
        # Intel specific optimizations
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma -msse4.2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -falign-functions=16 -falign-loops=16")
        
        # x86_64 specific optimizations
        add_compile_definitions(HFX_INTEL_X86_64=1)
        add_compile_definitions(HFX_AVX2_SUPPORT=1)
    endif()
    
    # Common Apple optimizations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector -fno-plt")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -fno-math-errno")
    
    # HFT-specific optimizations
    add_compile_definitions(HFX_APPLE_PLATFORM=1)
    add_compile_definitions(HFX_HIGH_RESOLUTION_TIMING=1)
    
else()
    # Generic optimization flags for non-Apple platforms
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -finline-functions -funroll-loops")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-exceptions -fno-rtti")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -falign-functions=32 -falign-loops=32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector")
endif()

# Enable visualization components
option(HFX_ENABLE_VISUALIZATION "Enable real-time visualization components" ON)
option(HFX_ENABLE_WEBSOCKET_SERVER "Enable WebSocket server for web dashboard" ON)
option(HFX_ENABLE_TERMINAL_DASHBOARD "Enable terminal dashboard" ON)
option(HFX_ENABLE_IMGUI_DASHBOARD "Enable ImGui native dashboard" ON)

# Enable AI components
option(HFX_ENABLE_AI "Enable AI trading components" ON)

if(HFX_ENABLE_VISUALIZATION)
    message(STATUS "🎨 Visualization components enabled")
    add_compile_definitions(HFX_VISUALIZATION_ENABLED=1)
endif()

# Enable all warnings and treat as errors for production code
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-missing-braces")

# Debug builds
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer")
    add_compile_definitions(HFX_DEBUG_MODE=1)
    message(STATUS "🐛 Debug mode enabled")
endif()

# Find required system libraries
find_package(PkgConfig REQUIRED)

# Handle threading
find_package(Threads REQUIRED)
if(APPLE)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

# Python for quantitative libraries
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
if(Python3_FOUND)
    message(STATUS "🐍 Python ${Python3_VERSION} found")
endif()

# High-performance third-party libraries
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third-party)

# Add subdirectories for each module (only include existing modules)
add_subdirectory(hfx-core)
add_subdirectory(hfx-net)
add_subdirectory(hfx-log)
add_subdirectory(hfx-ultra)
add_subdirectory(hfx-api)

if(HFX_ENABLE_AI)
    add_subdirectory(hfx-ai)
    message(STATUS "🤖 AI trading components enabled")
endif()

# Main executable target
add_executable(hydraflow-x 
    src/main.cpp
)

# Test executable for comprehensive system testing (commented out for now)
# add_executable(test-all-systems 
#     test_all_systems.cpp
# )

# Simple system test executable (commented out for now)
# add_executable(simple-test 
#     simple_system_test.cpp
# )

# Set output directory based on architecture
if(CMAKE_OSX_ARCHITECTURES)
    set_target_properties(hydraflow-x PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT_NAME "hydraflow-x"
    )
endif()

# Link all modules (only existing ones)
target_link_libraries(hydraflow-x PRIVATE
    hfx-core
    hfx-net
    hfx-log
    hfx-ultra
    hfx-api
    hfx-risk
    hfx-auth
    # hfx-monitor  # Temporarily disabled due to compilation issues
    hfx-security
    hfx-mempool
    # hfx-chain  # Temporarily disabled to test if ChainManager is causing build spam
    hfx-viz
    Threads::Threads
    ${Python3_LIBRARIES}
)

if(HFX_ENABLE_AI)
    target_link_libraries(hydraflow-x PRIVATE hfx-ai)
endif()

# Link libraries for test executable (commented out)
# target_link_libraries(test-all-systems PRIVATE
#     hfx-core
#     hfx-net
#     hfx-log
#     hfx-ultra
#     hfx-api
#     Threads::Threads
#     ${Python3_LIBRARIES}
# )

# if(HFX_ENABLE_AI)
#     target_link_libraries(test-all-systems PRIVATE hfx-ai)
# endif()

# Link libraries for simple test executable (commented out)
# target_link_libraries(simple-test PRIVATE
#     hfx-core
#     hfx-net
#     hfx-log
#     hfx-ultra
#     hfx-api
#     Threads::Threads
# )

# if(HFX_ENABLE_AI)
#     target_link_libraries(simple-test PRIVATE hfx-ai)
# endif()

# Apple-specific linking
if(APPLE)
    # Find Apple frameworks
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(IOKIT IOKit)
    find_library(SECURITY Security)
    find_library(SYSTEM_CONFIGURATION SystemConfiguration)
    find_library(ACCELERATE Accelerate)
    find_library(METAL Metal)
    find_library(METALKIT MetalKit)
    find_library(CORE_GRAPHICS CoreGraphics)
    find_library(QUARTZ_CORE QuartzCore)
    
    target_link_libraries(hydraflow-x PRIVATE
        ${CORE_FOUNDATION}
        ${IOKIT}
        ${SECURITY}
        ${SYSTEM_CONFIGURATION}
        ${ACCELERATE}
        ${CORE_GRAPHICS}
        ${QUARTZ_CORE}
    )
    
    # Only link Metal frameworks if they're available and not conflicting
    if(HFX_ENABLE_IMGUI_DASHBOARD AND METAL AND METALKIT)
        target_link_libraries(hydraflow-x PRIVATE ${METAL} ${METALKIT})
        add_compile_definitions(HFX_METAL_SUPPORT=1)
        message(STATUS "🔩 Metal framework support enabled")
    endif()
endif()

# Include directories for all modules
target_include_directories(hydraflow-x PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${Python3_INCLUDE_DIRS}
)

# Include directories for test executables (commented out)
# target_include_directories(test-all-systems PRIVATE
#     ${CMAKE_SOURCE_DIR}
#     ${Python3_INCLUDE_DIRS}
# )

# target_include_directories(simple-test PRIVATE
#     ${CMAKE_SOURCE_DIR}
# )

# Apple-specific linking for test executables (commented out)
# if(APPLE)
#     target_link_libraries(test-all-systems PRIVATE
#         ${CORE_FOUNDATION}
#         ${IOKIT}
#         ${SECURITY}
#         ${SYSTEM_CONFIGURATION}
#         ${ACCELERATE}
#         ${CORE_GRAPHICS}
#         ${QUARTZ_CORE}
#     )
    
#     target_link_libraries(simple-test PRIVATE
#         ${CORE_FOUNDATION}
#         ${IOKIT}
#         ${SECURITY}
#         ${SYSTEM_CONFIGURATION}
#         ${ACCELERATE}
#         ${CORE_GRAPHICS}
#         ${QUARTZ_CORE}
#     )
# endif()

# Compile definitions
target_compile_definitions(hydraflow-x PRIVATE
    HFX_VERSION_MAJOR=1
    HFX_VERSION_MINOR=0
    HFX_VERSION_PATCH=0
    HFX_BUILD_TIMESTAMP="${CMAKE_CURRENT_TIMESTAMP}"
)

# Compile definitions for test executable (commented out)
# target_compile_definitions(test-all-systems PRIVATE
#     HFX_VERSION_MAJOR=1
#     HFX_VERSION_MINOR=0
#     HFX_VERSION_PATCH=0
#     HFX_BUILD_TIMESTAMP="${CMAKE_CURRENT_TIMESTAMP}"
# )

# Enable AI features for test executables if available (commented out)
# if(HFX_ENABLE_AI)
#     target_compile_definitions(test-all-systems PRIVATE HFX_ENABLE_AI=1)
#     target_compile_definitions(simple-test PRIVATE HFX_ENABLE_AI=1)
# endif()

# Compile definitions for simple test executable (commented out)
# target_compile_definitions(simple-test PRIVATE
#     HFX_VERSION_MAJOR=1
#     HFX_VERSION_MINOR=0
#     HFX_VERSION_PATCH=0
# )

# Installation rules
install(TARGETS hydraflow-x 
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# Copy configuration files
install(DIRECTORY configs/
    DESTINATION etc/hydraflow-x
    COMPONENT Configuration
)

# Copy web dashboard
if(EXISTS ${CMAKE_SOURCE_DIR}/web-dashboard/out)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/web-dashboard/out/
        DESTINATION share/hydraflow-x/web
        COMPONENT WebDashboard
    )
endif()

# Build summary
message(STATUS "")
message(STATUS "🚀 HydraFlow-X Build Configuration Summary")
message(STATUS "==========================================")
message(STATUS "• Target Architecture: ${HFX_TARGET_ARCH}")
message(STATUS "• Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "• C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "• Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "• Python: ${Python3_VERSION}")
message(STATUS "• Visualization: ${HFX_ENABLE_VISUALIZATION}")
message(STATUS "• WebSocket Server: ${HFX_ENABLE_WEBSOCKET_SERVER}")
message(STATUS "• Terminal Dashboard: ${HFX_ENABLE_TERMINAL_DASHBOARD}")
message(STATUS "• ImGui Dashboard: ${HFX_ENABLE_IMGUI_DASHBOARD}")
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(STATUS "• Link-Time Optimization: ON")
endif()
message(STATUS "==========================================")
message(STATUS "")
