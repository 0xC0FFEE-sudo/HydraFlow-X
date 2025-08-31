# HydraFlow-X AI Trading Components
# Ultra-Low Latency AI-Driven Trading System

cmake_minimum_required(VERSION 3.20)

# AI Trading Module
add_library(hfx-ai STATIC
    src/sentiment_engine.cpp
    src/llm_decision_system_core.cpp
    src/data_feeds_manager.cpp
    src/research_agent.cpp
    src/signal_processor.cpp
    src/crypto_market_analyzer.cpp
    src/ai_strategy_generator.cpp
    src/backtesting_engine_impl.cpp
    src/autonomous_research_engine.cpp
    src/api_integration_manager.cpp
    src/sentiment_to_execution_pipeline.cpp
    src/real_time_data_aggregator.cpp
)

target_include_directories(hfx-ai PUBLIC
    include
    ${CMAKE_SOURCE_DIR}
    ${Python3_INCLUDE_DIRS}
)

# Find required libraries for API integration  
# Handle keg-only curl on macOS
if(APPLE)
    set(CURL_ROOT "/usr/local/opt/curl")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/curl")
endif()

find_package(CURL REQUIRED)
find_package(nlohmann_json QUIET)

# Fallback for nlohmann_json if not found via package manager
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR
        NAMES nlohmann/json.hpp
        PATHS /usr/local/include /opt/homebrew/include
    )
    if(NLOHMANN_JSON_INCLUDE_DIR)
        add_library(nlohmann_json INTERFACE)
        target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
        set(nlohmann_json_FOUND TRUE)
    endif()
endif()

# Link core HFT libraries
target_link_libraries(hfx-ai PRIVATE
    hfx-core
    hfx-net
    hfx-log
    ${Python3_LIBRARIES}
    CURL::libcurl
)

# Link nlohmann/json if found, otherwise use header-only
if(nlohmann_json_FOUND)
    target_link_libraries(hfx-ai PRIVATE nlohmann_json::nlohmann_json)
else()
    # Fallback to system includes or bundled headers
    target_compile_definitions(hfx-ai PRIVATE HFX_JSON_HEADER_ONLY=1)
endif()

# Enable AI-specific optimizations
target_compile_definitions(hfx-ai PRIVATE
    HFX_AI_ENABLED=1
    HFX_SENTIMENT_ANALYSIS=1
    HFX_LLM_INTEGRATION=1
    HFX_AUTONOMOUS_RESEARCH=1
)

# Python embedding for ML models
if(Python3_FOUND)
    target_compile_definitions(hfx-ai PRIVATE HFX_PYTHON_EMBED=1)
endif()

# Architecture-specific AI optimizations
if(HFX_APPLE_SILICON)
    # Apple Neural Engine integration potential
    target_compile_definitions(hfx-ai PRIVATE HFX_APPLE_ML=1)
elseif(HFX_INTEL_X86_64)
    # Intel MKL-DNN optimizations
    target_compile_definitions(hfx-ai PRIVATE HFX_INTEL_MKL=1)
endif()

message(STATUS "🤖 AI Trading Components configured")
message(STATUS "   • Sentiment Analysis Engine")
message(STATUS "   • LLM Decision System") 
message(STATUS "   • Multi-Source Data Feeds")
message(STATUS "   • Autonomous Research Agent")
message(STATUS "   • Crypto Market Analyzer")
message(STATUS "   • AI Strategy Generator")
