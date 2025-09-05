# Production Trading Engine Module
cmake_minimum_required(VERSION 3.20)

# Create trading engine library
add_library(trading-engine STATIC
    src/production_trader.cpp
)

target_include_directories(trading-engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE  
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(trading-engine
    PRIVATE
        pthread
)

# C++23 requirements
target_compile_features(trading-engine PRIVATE cxx_std_23)
target_compile_options(trading-engine PRIVATE
    -Wall -Wextra -O3 -march=native
    -ffast-math -funroll-loops
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
