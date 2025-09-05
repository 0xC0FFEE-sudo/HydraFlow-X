cmake_minimum_required(VERSION 3.20)
project(hfx-monitor VERSION 1.0.0 LANGUAGES CXX)

# Find required packages
find_package(Threads REQUIRED)

# Create static library
add_library(hfx-monitor STATIC
    src/metrics_collector.cpp
    src/health_checker.cpp
    src/performance_monitor.cpp
    src/alert_manager.cpp
    src/prometheus_exporter.cpp
)

# Add alert_manager.cpp to sources
target_sources(hfx-monitor PRIVATE src/alert_manager.cpp)

# Include directories
target_include_directories(hfx-monitor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
    ${CMAKE_SOURCE_DIR}/hfx-log/include
)

# Link dependencies
target_link_libraries(hfx-monitor PRIVATE
    hfx-core
    hfx-log
    Threads::Threads
)

# Compile definitions
target_compile_definitions(hfx-monitor PRIVATE
    HAS_PROMETHEUS_SUPPORT=1
)

# Set C++ standard
target_compile_features(hfx-monitor PUBLIC cxx_std_23)
