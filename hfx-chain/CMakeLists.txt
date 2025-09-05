# HFX-Chain: Multi-chain light clients and builder integration
add_library(hfx-chain STATIC
    src/chain_manager.cpp
    src/ethereum_client.cpp
    src/arbitrum_client.cpp
    src/optimism_client.cpp
    src/base_client.cpp
    src/builder_interface.cpp
    src/private_relay.cpp
    src/web3_provider.cpp
)

target_include_directories(hfx-chain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hfx-chain PUBLIC
    hfx-core
    hfx-net
)

target_compile_features(hfx-chain PUBLIC cxx_std_23)

# Find curl for HTTP RPC calls
find_package(CURL REQUIRED)
if(CURL_FOUND)
    target_include_directories(hfx-chain PRIVATE ${CURL_INCLUDE_DIRS})
    target_link_libraries(hfx-chain PRIVATE ${CURL_LIBRARIES})
    target_compile_definitions(hfx-chain PRIVATE HAS_CURL=1)
endif()

# JSON library for RPC communication
set(JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third-party/json)
if(NOT EXISTS ${JSON_DIR})
    message(STATUS "nlohmann/json not found, downloading...")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/nlohmann/json.git ${JSON_DIR}
    )
endif()

target_include_directories(hfx-chain PRIVATE ${JSON_DIR}/include)

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-chain-tests
        tests/test_chain_manager.cpp
        tests/test_ethereum_client.cpp
        tests/test_builder_interface.cpp
    )
    
    target_link_libraries(hfx-chain-tests PRIVATE
        hfx-chain
        gtest_main
    )
    
    gtest_discover_tests(hfx-chain-tests)
endif()
