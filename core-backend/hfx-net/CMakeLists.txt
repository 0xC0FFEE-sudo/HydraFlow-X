# HFX-Net: Ultra-low latency networking with macOS kqueue optimization
add_library(hfx-net STATIC
    src/network_manager.cpp
    src/kqueue_handler.cpp
    src/quic_client.cpp
    src/websocket_client.cpp
    src/tls_manager.cpp
    src/connection_pool.cpp
)

target_include_directories(hfx-net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hfx-net PUBLIC
    hfx-core
)

target_compile_features(hfx-net PUBLIC cxx_std_23)

# macOS-specific networking libraries
if(APPLE)
    target_compile_definitions(hfx-net PRIVATE MACOS_KQUEUE_OPTIMIZED=1)
    target_link_libraries(hfx-net PRIVATE
        ${CORE_FOUNDATION}
        ${SECURITY}
        ${SYSTEM_CONFIGURATION}
    )
endif()

# Find and link networking libraries
find_package(OpenSSL REQUIRED)
target_link_libraries(hfx-net PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-net-tests
        tests/test_network_manager.cpp
        tests/test_kqueue_handler.cpp
        tests/test_connection_pool.cpp
    )
    
    target_link_libraries(hfx-net-tests PRIVATE
        hfx-net
        gtest_main
    )
    
    gtest_discover_tests(hfx-net-tests)
endif()
