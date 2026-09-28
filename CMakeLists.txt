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

# Keep SDL self-contained and quiet. We still compile Log Sift itself with
# strict warnings below; these settings only avoid third-party SDK noise.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL3)

if(APPLE)
    foreach(_sdl_target SDL3-static SDL3-shared)
        if(TARGET ${_sdl_target})
            target_compile_options(${_sdl_target} PRIVATE -Wno-deprecated-declarations)
        endif()
    endforeach()
endif()

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

# Use the OS libcurl on macOS; build a static Schannel-backed copy on
# Windows so a separate curl executable or OpenSSL install is unnecessary.
if(WIN32)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF)
    FetchContent_Declare(curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_17_0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(curl)
    unset(BUILD_TESTING)
else()
    find_package(CURL REQUIRED)
endif()

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

set(LOGSIFT_SOURCE_FRAGMENTS
    src/platform/windows.inl
    src/platform/clipboard.inl
    src/config/config_types.h
    src/core/process.h
    src/core/task.h
    src/ui/style.h
    src/core/activity.inl
    src/config/config_store.h
    src/core/output_prefs.inl
    src/audio/notifications.inl
    src/core/stats_history.h
    src/core/atomic_file.h
    src/core/sift_types.inl
    src/profiles/profiles.inl
    src/profiles/profiles.h
    src/model/client.h
    src/model/fast_path.inl
    src/model/vision.inl
    src/io/input.inl
    src/ui/widgets.inl
    src/cli/help.inl
    src/cli/run.inl
    src/app/state.inl
    src/model/http.h
    src/ui/popup_timer.h
    src/app/setup.inl
    src/app/events_tray_watch.inl
    src/app/events_sdl_async.inl
    src/ui/main_header.inl
    src/ui/sift_tab.inl
    src/ui/recents_tab.inl
    src/ui/settings_tab.inl
    src/ui/main_render.inl
    src/ui/toast_lifecycle.inl
    src/ui/toast_layout.inl
    src/ui/toast_content.inl
    src/ui/toast_footer.inl
    src/app/frame_wait.inl
    src/app/shutdown.inl
)
set_source_files_properties(${LOGSIFT_SOURCE_FRAGMENTS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(logsift
    main.cpp
    src/core/process.cpp
    src/config/config_store.cpp
    src/core/stats_history.cpp
    src/core/atomic_file.cpp
    src/model/http.cpp
    src/model/client.cpp
    src/profiles/profiles.cpp
    src/ui/popup_timer.cpp
    src/ui/style.cpp
    ${LOGSIFT_SOURCE_FRAGMENTS})
target_compile_definitions(logsift PRIVATE
    LOGSIFT_VERSION="${PROJECT_VERSION}")

if(APPLE)
    set(LOGSIFT_GENERATED_ICON_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated-icons")
    set(LOGSIFT_ICONSET_DIR "${LOGSIFT_GENERATED_ICON_DIR}/LogSift.iconset")
    set(LOGSIFT_APP_ICON "${LOGSIFT_GENERATED_ICON_DIR}/LogSift.icns")
    set(LOGSIFT_TRAY_ICON "${LOGSIFT_GENERATED_ICON_DIR}/LogSiftTray.png")

    add_executable(logsift_icon_renderer EXCLUDE_FROM_ALL
        assets/render_macos_icons.mm)
    target_compile_options(logsift_icon_renderer PRIVATE -fobjc-arc)
    target_link_libraries(logsift_icon_renderer PRIVATE "-framework Cocoa")

    add_custom_command(
        OUTPUT "${LOGSIFT_APP_ICON}" "${LOGSIFT_TRAY_ICON}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LOGSIFT_GENERATED_ICON_DIR}"
        COMMAND $<TARGET_FILE:logsift_icon_renderer> "${LOGSIFT_GENERATED_ICON_DIR}"
        COMMAND /usr/bin/iconutil -c icns "${LOGSIFT_ICONSET_DIR}" -o "${LOGSIFT_APP_ICON}"
        DEPENDS logsift_icon_renderer
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/render_macos_icons.mm"
        COMMENT "Rendering centered Log Sift app and menu bar icons"
        VERBATIM)

    add_custom_target(logsift_icons
        DEPENDS "${LOGSIFT_APP_ICON}" "${LOGSIFT_TRAY_ICON}")
    add_dependencies(logsift logsift_icons)

    set_source_files_properties(
        "${LOGSIFT_APP_ICON}"
        "${LOGSIFT_TRAY_ICON}"
        PROPERTIES
            GENERATED TRUE
            MACOSX_PACKAGE_LOCATION "Resources")

    set_target_properties(logsift PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "Log Sift"
        MACOSX_BUNDLE_BUNDLE_NAME "Log Sift"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.nocxr.logsift"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "LogSift.icns"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in")
    target_sources(logsift PRIVATE
        mac_tray.mm
        "${LOGSIFT_APP_ICON}"
        "${LOGSIFT_TRAY_ICON}")
    target_compile_options(logsift PRIVATE
        $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
endif()

target_link_libraries(logsift PRIVATE
    SDL3::SDL3
    logsift_imgui
    nlohmann_json::nlohmann_json
    CURL::libcurl)

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

include(CTest)
if(BUILD_TESTING)
    add_executable(logsift_core_tests
        tests/core_tests.cpp
        src/core/process.cpp
        src/config/config_store.cpp
        src/core/stats_history.cpp
        src/core/atomic_file.cpp
        src/profiles/profiles.cpp
        src/ui/popup_timer.cpp)
    target_include_directories(logsift_core_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(logsift_core_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME logsift_core COMMAND logsift_core_tests)
    add_test(NAME logsift_cli_help COMMAND logsift --help)
    set_tests_properties(logsift_cli_help PROPERTIES PASS_REGULAR_EXPRESSION "USAGE")
    add_test(NAME logsift_cli_msvc COMMAND logsift --cli
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/msvc.log" --json --profile msvc)
    set_tests_properties(logsift_cli_msvc PROPERTIES PASS_REGULAR_EXPRESSION "C2143")
endif()
