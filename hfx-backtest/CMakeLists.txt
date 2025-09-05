cmake_minimum_required(VERSION 3.20)
project(hfx-backtest VERSION 1.0.0 LANGUAGES CXX)

# Create static library
add_library(hfx-backtest STATIC
    src/backtest_data.cpp
    src/trading_strategy.cpp
    src/backtest_engine.cpp
    src/data_sources.cpp
)

# Include directories
target_include_directories(hfx-backtest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
    ${CMAKE_SOURCE_DIR}/hfx-log/include
)

# Link dependencies
target_link_libraries(hfx-backtest PRIVATE
    hfx-core
    hfx-log
)

# Set C++ standard
target_compile_features(hfx-backtest PUBLIC cxx_std_23)