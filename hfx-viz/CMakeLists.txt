# HydraFlow-X Visualization Module
# Ultra-low latency real-time visualization for HFT systems

add_library(hfx-viz STATIC
    src/telemetry_engine.cpp
    src/websocket_server.cpp
    src/terminal_dashboard.cpp
    src/imgui_dashboard.cpp
    src/performance_metrics.cpp
    src/data_streamer.cpp
)

target_include_directories(hfx-viz PUBLIC include)

target_compile_features(hfx-viz PUBLIC cxx_std_23)

# Find and link visualization dependencies
find_package(PkgConfig REQUIRED)

# ImGui for native dashboard
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third-party/imgui)
if(NOT EXISTS ${IMGUI_DIR})
    message(STATUS "ImGui not found, downloading...")
    file(DOWNLOAD 
        https://github.com/ocornut/imgui/archive/refs/heads/docking.zip
        ${CMAKE_CURRENT_BINARY_DIR}/imgui.zip)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf imgui.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/imgui-docking ${IMGUI_DIR})
endif()

# WebSocket++ for web streaming (header-only)
set(WEBSOCKETPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third-party/websocketpp)
if(NOT EXISTS ${WEBSOCKETPP_DIR})
    message(STATUS "WebSocket++ not found, downloading...")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/zaphoyd/websocketpp.git ${WEBSOCKETPP_DIR}
    )
endif()

# JSON for modern C++ (header-only)
set(JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third-party/json)
if(NOT EXISTS ${JSON_DIR})
    message(STATUS "nlohmann/json not found, downloading...")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/nlohmann/json.git ${JSON_DIR}
    )
endif()

# ImGui sources
target_sources(hfx-viz PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include directories
target_include_directories(hfx-viz PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${WEBSOCKETPP_DIR}
    ${JSON_DIR}/include
)

# Platform-specific visualization optimizations
if(APPLE)
    target_compile_definitions(hfx-viz PRIVATE MACOS_METAL_VIZ=1)
    find_library(OPENGL_LIBRARY OpenGL)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(hfx-viz PRIVATE 
        ${OPENGL_LIBRARY} 
        ${COCOA_LIBRARY} 
        ${IOKIT_LIBRARY} 
        ${COREVIDEO_LIBRARY}
    )
endif()

# GLFW for windowing
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)
endif()

target_link_libraries(hfx-viz PRIVATE glfw)

# Threading support
find_package(Threads REQUIRED)
target_link_libraries(hfx-viz PRIVATE Threads::Threads)

# Boost for ASIO (WebSocket server)
find_package(Boost COMPONENTS system thread QUIET)
if(Boost_FOUND)
    target_link_libraries(hfx-viz PRIVATE Boost::system Boost::thread)
    target_compile_definitions(hfx-viz PRIVATE HAS_BOOST_ASIO=1)
endif()
