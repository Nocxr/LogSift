cmake_minimum_required(VERSION 3.25)
project(LogSift VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    enable_language(OBJCXX)
endif()

include(FetchContent)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

add_library(logsift_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(logsift_imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/misc/cpp
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(logsift_imgui PUBLIC SDL3::SDL3)

add_executable(logsift main.cpp)
if(APPLE)
    target_sources(logsift PRIVATE mac_tray.mm)
endif()

target_link_libraries(logsift PRIVATE
    SDL3::SDL3
    logsift_imgui
    nlohmann_json::nlohmann_json)

add_custom_command(TARGET logsift POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles"
        "$<TARGET_FILE_DIR:logsift>/log-sift-profiles"
    COMMENT "Copying Log Sift profiles")

if(MSVC)
    target_compile_options(logsift PRIVATE /W4 /permissive-)
else()
    target_compile_options(logsift PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(APPLE)
    target_link_libraries(logsift PRIVATE "-framework Cocoa")
endif()

if(WIN32)
    target_compile_definitions(logsift PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0A00)
    target_link_libraries(logsift PRIVATE winmm)
endif()
