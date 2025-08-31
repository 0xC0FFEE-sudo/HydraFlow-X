# HFX-Core: Event engine, time wheel, memory management, lock-free data structures
add_library(hfx-core STATIC
    src/event_engine.cpp
    src/time_wheel.cpp
    src/memory_pool.cpp
    src/lockfree_queue.cpp
    src/state_snapshot.cpp
    src/thread_utils.cpp
)

target_include_directories(hfx-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(hfx-core PUBLIC cxx_std_23)

# Apple-specific optimizations
if(APPLE)
    target_compile_definitions(hfx-core PRIVATE APPLE_SILICON_OPTIMIZED=1)
    target_link_libraries(hfx-core PRIVATE
        ${CORE_FOUNDATION}
        ${IOKIT}
    )
endif()

# Unit tests
if(BUILD_TESTING)
    add_executable(hfx-core-tests
        tests/test_event_engine.cpp
        tests/test_time_wheel.cpp
        tests/test_memory_pool.cpp
        tests/test_lockfree_queue.cpp
    )
    
    target_link_libraries(hfx-core-tests PRIVATE
        hfx-core
        gtest_main
    )
    
    gtest_discover_tests(hfx-core-tests)
endif()
