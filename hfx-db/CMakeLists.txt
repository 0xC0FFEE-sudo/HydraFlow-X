cmake_minimum_required(VERSION 3.20)
project(hfx-db VERSION 1.0.0 LANGUAGES CXX)

# Find PostgreSQL
find_package(PostgreSQL REQUIRED)
if(NOT PostgreSQL_FOUND)
    message(FATAL_ERROR "PostgreSQL not found. Please install PostgreSQL development libraries.")
endif()

# Find Threads
find_package(Threads REQUIRED)

# Find ClickHouse client library (if available)
find_package(ClickHouse QUIET)
if(NOT ClickHouse_FOUND)
    message(WARNING "ClickHouse client library not found. ClickHouse functionality will be disabled.")
    set(HAS_CLICKHOUSE OFF)
else()
    set(HAS_CLICKHOUSE ON)
endif()

# Create library
add_library(hfx-db STATIC
    src/database_connection.cpp
    src/postgresql_connection.cpp
    src/clickhouse_connection.cpp
    src/trade_repository.cpp
    src/market_data_repository.cpp
    src/position_repository.cpp
    src/analytics_repository.cpp
    src/risk_metrics_repository.cpp
    src/liquidity_pool_repository.cpp
    src/performance_metrics_repository.cpp
    src/database_manager.cpp
)

# Include directories
target_include_directories(hfx-db
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PostgreSQL_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(hfx-db
    PUBLIC
        ${PostgreSQL_LIBRARIES}
        hfx-core
        hfx-log
    PRIVATE
        Threads::Threads
)

# Compile definitions
target_compile_definitions(hfx-db
    PRIVATE
        HAS_CLICKHOUSE=$<BOOL:${HAS_CLICKHOUSE}>
)

# Set C++ standard
target_compile_features(hfx-db PUBLIC cxx_std_23)

# Compiler warnings
if(MSVC)
    target_compile_options(hfx-db PRIVATE /W4)
else()
    target_compile_options(hfx-db PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install headers
install(DIRECTORY include/
    DESTINATION include/hfx-db
    FILES_MATCHING PATTERN "*.hpp"
)

# Install library
install(TARGETS hfx-db
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
