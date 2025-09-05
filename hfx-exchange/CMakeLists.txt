# HFX Exchange Integration Module
# Real exchange API integrations for trading

cmake_minimum_required(VERSION 3.20)
project(hfx-exchange)

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

# Exchange API library
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Remove any stub files if they exist
list(FILTER SOURCES EXCLUDE REGEX ".*stub.*")

add_library(hfx-exchange ${SOURCES})

target_include_directories(hfx-exchange PUBLIC include)
target_include_directories(hfx-exchange PRIVATE ../core-backend/hfx-log/include)
target_include_directories(hfx-exchange PRIVATE ../hfx-chain/include)

target_link_libraries(hfx-exchange 
    PRIVATE
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Set library properties
set_target_properties(hfx-exchange PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    POSITION_INDEPENDENT_CODE ON
)

# Install rules
install(TARGETS hfx-exchange
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/hfx-exchange
    FILES_MATCHING PATTERN "*.hpp"
)
