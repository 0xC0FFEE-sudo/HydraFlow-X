# HydraFlow-X Ultra-Low Latency HFT Module
# Sub-microsecond execution engine for memecoin trading

cmake_minimum_required(VERSION 3.25)

# HFT Execution Module - Critical Path Components
add_library(hfx-hft STATIC
    src/replay_harness.cpp
    src/policy_engine.cpp
    src/signal_compressor.cpp
    src/consensus_engine.cpp
    src/impact_simulator.cpp
    src/slo_monitor.cpp
    src/market_simulator.cpp
    src/execution_engine.cpp
    src/memecoin_sniping.cpp
    src/memecoin_integrations.cpp
    src/mev_strategy.cpp
    src/hft_main.cpp
    src/dex_integration.cpp
    src/wallet_integration.cpp
    src/permit2_integration.cpp
    src/order_routing_engine.cpp
    src/jito_mev_integration.cpp
)

target_include_directories(hfx-hft PUBLIC
    include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/hfx-core/include
    ${CMAKE_SOURCE_DIR}/hfx-ai/include
)

target_compile_features(hfx-hft PUBLIC cxx_std_23)

# Ultra-aggressive optimization flags for HFT
target_compile_options(hfx-hft PRIVATE
    -O3 -DNDEBUG
    -fomit-frame-pointer
    -finline-functions
    -funroll-loops
    -fno-exceptions
    -fno-rtti
    -flto=thin
    -march=native
    -mtune=native
    -mavx2 -mfma
    -falign-functions=64  # Cache line alignment
    -falign-loops=64
    -fno-stack-protector
    -fno-plt
    -ffast-math
    -fno-math-errno
    -DLIKELY_BRANCH_PREDICTION=1
)

# HFT-specific compile definitions
target_compile_definitions(hfx-hft PRIVATE
    HFX_HFT_MODE=1
    HFX_ULTRA_LOW_LATENCY=1
    HFX_MEMECOIN_SNIPING=1
    HFX_MEV_ENABLED=1
    HFX_MICROSECOND_PRECISION=1
    HFX_LOCK_FREE_STRUCTURES=1
)

# Link dependencies
target_link_libraries(hfx-hft PUBLIC
    hfx-core
    Threads::Threads
)

# Threading and memory model optimizations
find_package(Threads REQUIRED)

# Optional: DPDK for kernel bypass (if available)
find_path(DPDK_INCLUDE_DIR rte_config.h PATH_SUFFIXES dpdk HINTS /usr/include /opt/dpdk/include)
if(DPDK_INCLUDE_DIR)
    target_include_directories(hfx-hft PRIVATE ${DPDK_INCLUDE_DIR})
    target_compile_definitions(hfx-hft PRIVATE HFX_DPDK_ENABLED=1)
    message(STATUS "🚀 DPDK found - enabling kernel bypass for ultra-low latency")
endif()

# Platform-specific optimizations
if(APPLE)
    target_compile_definitions(hfx-hft PRIVATE HFX_MACOS_OPTIMIZATIONS=1)
    target_link_libraries(hfx-hft PRIVATE
        "-framework CoreFoundation"
        "-framework IOKit"
        "-framework Security"
        "-framework SystemConfiguration"
    )
endif()

# Memory optimization flags
target_link_options(hfx-hft PRIVATE
    -flto=thin
    -Wl,-dead_strip  # Remove unused code
)
