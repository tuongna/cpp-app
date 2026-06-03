cmake_minimum_required(VERSION 3.16)
project(SteamClone VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2 /Os)
    else()
        add_compile_options(-O2 -Os)
    endif()
endif()

include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# cpp-httplib (header-only)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(httplib)

# webview (header-only)
FetchContent_Declare(
    webview
    GIT_REPOSITORY https://github.com/webview/webview.git
    GIT_TAG        0.12.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(webview)

set(SOURCES
    src/main.cpp
    src/Downloader.cpp
    src/Launcher.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${webview_SOURCE_DIR}
    ${httplib_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    nlohmann_json::nlohmann_json
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ole32
        shell32
        shlwapi
    )
    # Embed WebView2 bootstrap
    FetchContent_Declare(
        webview2
        URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2792.45"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE WEBVIEW_EDGE)
    # Hide console window in release
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(WEBKIT_LIBRARY WebKit REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY}
        ${WEBKIT_LIBRARY}
    )
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${WEBKIT2GTK_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GTK3_LIBRARIES}
        ${WEBKIT2GTK_LIBRARIES}
    )
    target_compile_options(${PROJECT_NAME} PRIVATE
        ${GTK3_CFLAGS_OTHER}
        ${WEBKIT2GTK_CFLAGS_OTHER}
    )
endif()

# Thread support
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Copy UI dist to output dir after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/../app-ui/.output/public"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ui"
    COMMENT "Copying UI static files..."
    VERBATIM
)
