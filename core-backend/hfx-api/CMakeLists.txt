# HydraFlow-X API Module
# RESTful API and WebSocket server for web dashboard communication

cmake_minimum_required(VERSION 3.20)

# Find required packages
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, using FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Find threading support
find_package(Threads REQUIRED)

# Create the hfx-api library
add_library(hfx-api STATIC
    src/rest_api_server.cpp
)

# Set target properties
target_include_directories(hfx-api PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hfx-api PUBLIC
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Link system libraries based on platform
if(UNIX)
    target_link_libraries(hfx-api PRIVATE
        pthread
    )
endif()

if(APPLE)
    # macOS specific libraries
    target_link_libraries(hfx-api PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
    )
endif()

# Set compile features
target_compile_features(hfx-api PUBLIC cxx_std_23)

# Set compile options
target_compile_options(hfx-api PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Set compile definitions
target_compile_definitions(hfx-api PRIVATE
    HFX_API_VERSION_MAJOR=1
    HFX_API_VERSION_MINOR=0
    HFX_API_VERSION_PATCH=0
)

# Platform specific definitions
if(WIN32)
    target_compile_definitions(hfx-api PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601
    )
endif()

# Debug/Release specific settings
target_compile_definitions(hfx-api PRIVATE
    $<$<CONFIG:Debug>:HFX_DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG>
)

# Installation
install(TARGETS hfx-api
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/hfx-api
    FILES_MATCHING PATTERN "*.hpp"
)
