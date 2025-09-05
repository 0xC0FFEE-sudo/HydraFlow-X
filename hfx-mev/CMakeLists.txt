# HFX MEV Protection Module
# Advanced MEV protection and bundle simulation

cmake_minimum_required(VERSION 3.20)
project(hfx-mev)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimizations for ultra-low latency
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto")

# Find dependencies
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(include)
include_directories(../core-backend/hfx-log/include)
include_directories(../hfx-chain/include)
include_directories(../hfx-risk/include)

# MEV Protection library
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Remove any stub files if they exist
list(FILTER SOURCES EXCLUDE REGEX ".*stub.*")

add_library(hfx-mev ${SOURCES})

target_include_directories(hfx-mev PUBLIC include)
target_include_directories(hfx-mev PRIVATE ../core-backend/hfx-log/include)
target_include_directories(hfx-mev PRIVATE ../hfx-chain/include)
target_include_directories(hfx-mev PRIVATE ../hfx-risk/include)

target_link_libraries(hfx-mev 
    PRIVATE
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Set library properties
set_target_properties(hfx-mev PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    POSITION_INDEPENDENT_CODE ON
)

# Install rules
install(TARGETS hfx-mev
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/hfx-mev
    FILES_MATCHING PATTERN "*.hpp"
)
