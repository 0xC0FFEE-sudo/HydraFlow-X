# HydraFlow-X Production Trading Engine
cmake_minimum_required(VERSION 3.20)
project(HydraFlow-X VERSION 1.0.0 LANGUAGES CXX)

# Build options
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_WEB_DASHBOARD "Build web dashboard" OFF)  
option(BUILD_CORE_BACKEND "Build core backend" ON)

# C++23 requirement
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler optimizations for production
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto")
set(CMAKE_BUILD_TYPE Release)

# Core backend modules
if(BUILD_CORE_BACKEND)
    # Add core modules
    add_subdirectory(core-backend/trading-engine)
    add_subdirectory(core-backend/exchanges)
    
    # Production main executable
    add_executable(hydraflow-x
        core-backend/production_main.cpp
    )
    
    target_link_libraries(hydraflow-x
        PRIVATE
            trading-engine
            exchanges
            pthread
    )
    
    target_compile_features(hydraflow-x PRIVATE cxx_std_23)
    target_compile_options(hydraflow-x PRIVATE
        -Wall -Wextra -O3 -march=native
    )
    
    # Install target
    install(TARGETS hydraflow-x
        RUNTIME DESTINATION bin
    )
endif()

# Web dashboard (optional)
if(BUILD_WEB_DASHBOARD)
    add_subdirectory(web-dashboard)
endif()

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()