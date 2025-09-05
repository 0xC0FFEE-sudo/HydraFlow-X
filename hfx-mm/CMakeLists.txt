# HFX-MM: Market models and predictive analytics
add_library(hfx-mm STATIC
    src/gas_price_predictor.cpp
    src/inclusion_probability_model.cpp
    src/slippage_estimator.cpp
    src/queue_risk_assessor.cpp
    src/volatility_models.cpp
    src/correlation_tracker.cpp
)

target_include_directories(hfx-mm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
)

target_link_libraries(hfx-mm PUBLIC
    hfx-core
    ${Python3_LIBRARIES}
)

target_compile_features(hfx-mm PUBLIC cxx_std_23)

# Python integration for statistical models
target_include_directories(hfx-mm PRIVATE
    ${Python3_INCLUDE_DIRS}
)

# Apple ML acceleration
if(APPLE)
    target_compile_definitions(hfx-mm PRIVATE APPLE_STATS_ACCELERATED=1)
    target_link_libraries(hfx-mm PRIVATE
        ${ACCELERATE}
        ${CORE_ML}
    )
endif()

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-mm-tests
        tests/test_gas_price_predictor.cpp
        tests/test_inclusion_probability_model.cpp
        tests/test_slippage_estimator.cpp
    )
    
    target_link_libraries(hfx-mm-tests PRIVATE
        hfx-mm
        gtest_main
    )
    
    gtest_discover_tests(hfx-mm-tests)
endif()
