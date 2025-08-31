# HydraFlow-X Comprehensive Testing Framework
cmake_minimum_required(VERSION 3.20)

project(HydraFlow-X-Tests
    VERSION 1.0.0
    DESCRIPTION "Comprehensive testing suite for HydraFlow-X"
    LANGUAGES CXX
)

# Require C++23 for testing framework
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# Add the parent directory to include path
include_directories(${CMAKE_SOURCE_DIR}/..)

# Link with HydraFlow-X libraries
link_directories(${CMAKE_SOURCE_DIR}/../build/core-backend/hfx-ultra)
link_directories(${CMAKE_SOURCE_DIR}/../build/core-backend/hfx-core)
link_directories(${CMAKE_SOURCE_DIR}/../build/core-backend/hfx-net)
link_directories(${CMAKE_SOURCE_DIR}/../build/core-backend/hfx-log)
link_directories(${CMAKE_SOURCE_DIR}/../build/core-backend/hfx-ai)
link_directories(${CMAKE_SOURCE_DIR}/../build/core-backend/hfx-api)

# Compiler flags for testing
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -O3                          # Optimize for performance testing
        -DNDEBUG                     # Release mode for accurate benchmarks
        -march=native               # Use native CPU features
        -mtune=native               # Tune for native CPU
        -fno-omit-frame-pointer     # Better profiling support
    )
    
    # Additional flags for performance testing
    if(CMAKE_BUILD_TYPE STREQUAL "Performance")
        add_compile_options(
            -O3
            -DPERFORMANCE_MODE=1
            -ffast-math                 # Aggressive math optimizations
            -funroll-loops             # Loop unrolling
            -flto                      # Link-time optimization
        )
    endif()
endif()

# Apple-specific optimizations
if(APPLE)
    # Enable Apple Silicon optimizations if available
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        add_compile_definitions(HFX_APPLE_SILICON=1)
        add_compile_options(-mcpu=apple-a14)  # Target Apple Silicon
    else()
        add_compile_definitions(HFX_INTEL_X86_64=1)
        add_compile_options(-march=native)
    endif()
endif()

# Test configuration options
option(BUILD_UNIT_TESTS "Build unit tests" ON)
option(BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(BUILD_PERFORMANCE_TESTS "Build performance tests" ON)
option(ENABLE_TEST_COVERAGE "Enable code coverage for tests" OFF)
option(ENABLE_MEMORY_TESTING "Enable memory testing with valgrind/sanitizers" OFF)
option(ENABLE_PARALLEL_TESTS "Enable parallel test execution" ON)

# Code coverage setup
if(ENABLE_TEST_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    endif()
endif()

# Memory testing setup
if(ENABLE_MEMORY_TESTING)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# Function to create test executable
function(create_test_executable TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # Link with HydraFlow-X libraries
    target_link_libraries(${TEST_NAME} PRIVATE
        hfx-ultra
        hfx-core
        hfx-net
        hfx-log
        hfx-api
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
    )
    
    # Link with AI libraries if available
    if(EXISTS ${CMAKE_SOURCE_DIR}/../build/core-backend/hfx-ai)
        target_link_libraries(${TEST_NAME} PRIVATE hfx-ai)
    endif()
    
    # Apple-specific linking
    if(APPLE)
        find_library(CORE_FOUNDATION CoreFoundation)
        find_library(IOKIT IOKit)
        find_library(SECURITY Security)
        find_library(SYSTEM_CONFIGURATION SystemConfiguration)
        
        target_link_libraries(${TEST_NAME} PRIVATE
            ${CORE_FOUNDATION}
            ${IOKIT}
            ${SECURITY}
            ${SYSTEM_CONFIGURATION}
        )
    endif()
    
    # Set output directory
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        OUTPUT_NAME ${TEST_NAME}
    )
    
    # Add test to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # Set test properties
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 300  # 5 minutes default timeout
        LABELS "HydraFlow-X"
    )
endfunction()

# Unit Tests
if(BUILD_UNIT_TESTS)
    message(STATUS "🧪 Building unit tests")
    
    create_test_executable(test_security_manager unit/test_security_manager.cpp)
    set_tests_properties(test_security_manager PROPERTIES
        LABELS "unit;security"
        TIMEOUT 120
    )
    
    # Additional unit tests can be added here
    # create_test_executable(test_trading_engine unit/test_trading_engine.cpp)
    # create_test_executable(test_v3_tick_engine unit/test_v3_tick_engine.cpp)
    # create_test_executable(test_mev_shield unit/test_mev_shield.cpp)
endif()

# Integration Tests
if(BUILD_INTEGRATION_TESTS)
    message(STATUS "🔄 Building integration tests")
    
    create_test_executable(test_trading_flow integration/test_trading_flow.cpp)
    set_tests_properties(test_trading_flow PROPERTIES
        LABELS "integration;trading;end-to-end"
        TIMEOUT 600  # 10 minutes for integration tests
    )
    
    # Disable parallel execution for integration tests
    if(NOT ENABLE_PARALLEL_TESTS)
        set_tests_properties(test_trading_flow PROPERTIES
            RUN_SERIAL TRUE
        )
    endif()
endif()

# Performance Tests
if(BUILD_PERFORMANCE_TESTS)
    message(STATUS "⚡ Building performance tests")
    
    create_test_executable(test_basic_benchmarks performance/test_basic_benchmarks.cpp)
    set_tests_properties(test_basic_benchmarks PROPERTIES
        LABELS "performance;basic;computational"
        TIMEOUT 600  # 10 minutes for basic performance tests
    )
    
    create_test_executable(test_latency_benchmarks performance/test_latency_benchmarks.cpp)
    set_tests_properties(test_latency_benchmarks PROPERTIES
        LABELS "performance;latency;benchmark"
        TIMEOUT 1800  # 30 minutes for performance tests
    )
    
    # Performance tests should run in isolation
    set_tests_properties(test_basic_benchmarks PROPERTIES
        RUN_SERIAL TRUE
    )
    set_tests_properties(test_latency_benchmarks PROPERTIES
        RUN_SERIAL TRUE
    )
endif()

# Custom test targets
add_custom_target(run-unit-tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L unit --output-on-failure
    DEPENDS ${UNIT_TEST_TARGETS}
    COMMENT "Running unit tests"
)

add_custom_target(run-integration-tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L integration --output-on-failure
    DEPENDS ${INTEGRATION_TEST_TARGETS}
    COMMENT "Running integration tests"
)

add_custom_target(run-performance-tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L performance --output-on-failure
    DEPENDS ${PERFORMANCE_TEST_TARGETS}
    COMMENT "Running performance tests"
)

add_custom_target(run-all-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running all tests"
)

# Test reporting targets
add_custom_target(test-report
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --output-junit test_results.xml
    COMMENT "Running tests and generating XML report"
)

# Code coverage target
if(ENABLE_TEST_COVERAGE)
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND ${LCOV_PATH} --list coverage.info
            COMMAND ${GENHTML_PATH} -o coverage_report coverage.info
            COMMENT "Generating code coverage report"
        )
        
        message(STATUS "📊 Code coverage enabled - use 'make coverage' to generate report")
    else()
        message(WARNING "Code coverage tools not found")
    endif()
endif()

# Memory testing target
if(ENABLE_MEMORY_TESTING)
    find_program(VALGRIND_PATH valgrind)
    
    if(VALGRIND_PATH)
        add_custom_target(memcheck
            COMMAND ${CMAKE_CTEST_COMMAND} -T memcheck
            COMMENT "Running memory tests with valgrind"
        )
        
        message(STATUS "🔍 Memory testing enabled - use 'make memcheck' to run")
    else()
        message(STATUS "🔍 Memory testing enabled via sanitizers")
    endif()
endif()

# Benchmark comparison target
add_custom_target(benchmark-compare
    COMMAND ${CMAKE_COMMAND} -E echo "Running baseline performance benchmarks..."
    COMMAND ./bin/test_latency_benchmarks > baseline_results.txt 2>&1 || true
    COMMAND ${CMAKE_COMMAND} -E echo "Baseline results saved to baseline_results.txt"
    COMMENT "Running performance benchmarks for baseline comparison"
)

# Documentation generation for test framework
find_program(DOXYGEN_EXECUTABLE doxygen)
if(DOXYGEN_EXECUTABLE)
    add_custom_target(test-docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating test framework documentation"
    )
endif()

# Install test executables (optional)
if(BUILD_UNIT_TESTS)
    install(TARGETS test_security_manager
        RUNTIME DESTINATION bin/tests
        COMPONENT tests
    )
endif()

if(BUILD_INTEGRATION_TESTS)
    install(TARGETS test_trading_flow
        RUNTIME DESTINATION bin/tests
        COMPONENT tests
    )
endif()

if(BUILD_PERFORMANCE_TESTS)
    install(TARGETS test_latency_benchmarks
        RUNTIME DESTINATION bin/tests
        COMPONENT tests
    )
endif()

# Install test configuration files
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.json
    DESTINATION etc/hydraflow-x/tests
    COMPONENT tests
    OPTIONAL
)

# Summary
message(STATUS "")
message(STATUS "🧪 HydraFlow-X Testing Framework Configuration")
message(STATUS "===============================================")
message(STATUS "Unit Tests:        ${BUILD_UNIT_TESTS}")
message(STATUS "Integration Tests: ${BUILD_INTEGRATION_TESTS}")
message(STATUS "Performance Tests: ${BUILD_PERFORMANCE_TESTS}")
message(STATUS "Code Coverage:     ${ENABLE_TEST_COVERAGE}")
message(STATUS "Memory Testing:    ${ENABLE_MEMORY_TESTING}")
message(STATUS "Parallel Tests:    ${ENABLE_PARALLEL_TESTS}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  run-unit-tests        - Run all unit tests")
message(STATUS "  run-integration-tests - Run all integration tests")
message(STATUS "  run-performance-tests - Run all performance benchmarks")
message(STATUS "  run-all-tests         - Run all tests")
message(STATUS "  test-report           - Generate XML test report")
if(ENABLE_TEST_COVERAGE)
    message(STATUS "  coverage              - Generate code coverage report")
endif()
if(ENABLE_MEMORY_TESTING)
    message(STATUS "  memcheck              - Run memory tests")
endif()
message(STATUS "  benchmark-compare     - Run baseline performance comparison")
message(STATUS "")

# Enable testing with CTest
enable_testing()
