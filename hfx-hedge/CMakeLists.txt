# HFX-Hedge: Cross-venue hedging and atomic execution
add_library(hfx-hedge STATIC
    src/hedge_engine.cpp
    src/dag_executor.cpp
    src/cross_venue_router.cpp
    src/delta_neutralizer.cpp
    src/atomic_transaction_manager.cpp
    src/venue_connector.cpp
    src/execution_optimizer.cpp
)

target_include_directories(hfx-hedge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hfx-hedge PUBLIC
    hfx-core
    hfx-net
    hfx-chain
    hfx-strat
)

target_compile_features(hfx-hedge PUBLIC cxx_std_23)

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-hedge-tests
        tests/test_hedge_engine.cpp
        tests/test_dag_executor.cpp
        tests/test_cross_venue_router.cpp
        tests/test_atomic_transaction_manager.cpp
    )
    
    target_link_libraries(hfx-hedge-tests PRIVATE
        hfx-hedge
        gtest_main
    )
    
    gtest_discover_tests(hfx-hedge-tests)
endif()
