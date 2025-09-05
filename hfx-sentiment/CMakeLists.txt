cmake_minimum_required(VERSION 3.20)
project(hfx-sentiment VERSION 1.0.0 LANGUAGES CXX)

# Find required packages
find_package(CURL REQUIRED)

# Create static library
add_library(hfx-sentiment STATIC
    src/sentiment_analyzer.cpp
    src/sentiment_data_sources.cpp
    src/sentiment_engine.cpp
)

# Include directories
target_include_directories(hfx-sentiment PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/hfx-core/include
    ${CMAKE_SOURCE_DIR}/hfx-log/include
)

# Link dependencies
target_link_libraries(hfx-sentiment PRIVATE
    hfx-core
    hfx-log
    CURL::libcurl
)

# Set C++ standard
target_compile_features(hfx-sentiment PUBLIC cxx_std_23)
