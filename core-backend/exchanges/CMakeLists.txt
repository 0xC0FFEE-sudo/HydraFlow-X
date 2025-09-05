# Exchange Integrations Module
cmake_minimum_required(VERSION 3.20)

# Find required libraries
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

pkg_check_modules(LIBCURL REQUIRED libcurl)

# Create exchanges library
add_library(exchanges STATIC
    src/coinbase_exchange.cpp
)

target_include_directories(exchanges
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE  
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENSSL_INCLUDE_DIR}
        ${LIBCURL_INCLUDE_DIRS}
)

target_link_libraries(exchanges
    PRIVATE
        ${OPENSSL_LIBRARIES}
        ${LIBCURL_LIBRARIES}
        pthread
)

# C++23 requirements
target_compile_features(exchanges PRIVATE cxx_std_23)
target_compile_options(exchanges PRIVATE
    -Wall -Wextra -O3 -march=native
    ${LIBCURL_CFLAGS_OTHER}
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
