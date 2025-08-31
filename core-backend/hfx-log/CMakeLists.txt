# HFX-Log: High-performance binary logging and analytics
add_library(hfx-log STATIC
    src/logger.cpp
    src/binary_wal_writer.cpp
    src/attribution_engine.cpp
    src/performance_tracker.cpp
    src/telemetry_exporter.cpp
    src/log_analyzer.cpp
)

target_include_directories(hfx-log PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hfx-log PUBLIC
    hfx-core
)

target_compile_features(hfx-log PUBLIC cxx_std_23)

# Compression library for log files
find_library(ZLIB_LIBRARY z REQUIRED)
target_link_libraries(hfx-log PRIVATE ${ZLIB_LIBRARY})

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-log-tests
        tests/test_logger.cpp
        tests/test_binary_wal_writer.cpp
        tests/test_attribution_engine.cpp
        tests/test_performance_tracker.cpp
    )
    
    target_link_libraries(hfx-log-tests PRIVATE
        hfx-log
        gtest_main
    )
    
    gtest_discover_tests(hfx-log-tests)
endif()
