# HFX-Mempool: Mempool monitoring and intent parsing
add_library(hfx-mempool STATIC
    src/mempool_monitor.cpp
    src/intent_parser.cpp
    src/transaction_filter.cpp
    src/gas_estimator.cpp
    src/mev_detector.cpp
)

target_include_directories(hfx-mempool PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hfx-mempool PUBLIC
    hfx-core
    hfx-net
    hfx-chain
)

target_compile_features(hfx-mempool PUBLIC cxx_std_23)

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-mempool-tests
        tests/test_mempool_monitor.cpp
        tests/test_intent_parser.cpp
        tests/test_mev_detector.cpp
    )
    
    target_link_libraries(hfx-mempool-tests PRIVATE
        hfx-mempool
        gtest_main
    )
    
    gtest_discover_tests(hfx-mempool-tests)
endif()
