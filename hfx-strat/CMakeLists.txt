# HFX-Strat: Quantum-inspired trading strategies with multi-agent AI
add_library(hfx-strat STATIC
    src/strategy_engine.cpp
    src/oracle_arbitrage.cpp
    src/sequencer_queue_alpha.cpp
    src/liquidity_epoch_breathing.cpp
    src/quantum_optimizer.cpp
    src/multi_agent_system.cpp
    src/signal_generator.cpp
    src/ml_models.cpp
)

target_include_directories(hfx-strat PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hfx-strat PUBLIC
    hfx-core
    hfx-net
    hfx-mm
    ${Python3_LIBRARIES}
)

target_compile_features(hfx-strat PUBLIC cxx_std_23)

# Apple-specific ML acceleration
if(APPLE)
    target_compile_definitions(hfx-strat PRIVATE APPLE_ML_ACCELERATED=1)
    target_link_libraries(hfx-strat PRIVATE
        ${CORE_ML}
        ${METAL}
        ${METAL_PERFORMANCE_SHADERS}
        ${ACCELERATE}
    )
endif()

# Python integration for quantitative libraries
target_include_directories(hfx-strat PRIVATE
    ${Python3_INCLUDE_DIRS}
)

# Machine Learning libraries
find_package(PkgConfig REQUIRED)

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-strat-tests
        tests/test_strategy_engine.cpp
        tests/test_oracle_arbitrage.cpp
        tests/test_quantum_optimizer.cpp
        tests/test_multi_agent_system.cpp
    )
    
    target_link_libraries(hfx-strat-tests PRIVATE
        hfx-strat
        gtest_main
    )
    
    gtest_discover_tests(hfx-strat-tests)
endif()
