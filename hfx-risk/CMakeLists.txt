# HFX-Risk: AI-powered risk management with real-time circuit breakers
add_library(hfx-risk STATIC
    src/risk_manager.cpp
    src/circuit_breakers.cpp
    src/position_limits.cpp
    src/var_calculator.cpp
    src/anomaly_detector.cpp
    src/drawdown_guards.cpp
    src/ml_risk_models.cpp
)

target_include_directories(hfx-risk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-strat/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
)

target_link_libraries(hfx-risk PUBLIC
    hfx-core
    hfx-strat
)

target_compile_features(hfx-risk PUBLIC cxx_std_23)

# Apple-specific ML acceleration for risk models
if(APPLE)
    target_compile_definitions(hfx-risk PRIVATE APPLE_RISK_ML_ACCELERATED=1)
    target_link_libraries(hfx-risk PRIVATE
        ${CORE_ML}
        ${METAL_PERFORMANCE_SHADERS}
        ${ACCELERATE}
    )
endif()

# Statistical computing libraries
target_include_directories(hfx-risk PRIVATE
    ${Python3_INCLUDE_DIRS}
)

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-risk-tests
        tests/test_risk_manager.cpp
        tests/test_circuit_breakers.cpp
        tests/test_var_calculator.cpp
        tests/test_anomaly_detector.cpp
    )
    
    target_link_libraries(hfx-risk-tests PRIVATE
        hfx-risk
        gtest_main
    )
    
    gtest_discover_tests(hfx-risk-tests)
endif()
