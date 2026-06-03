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
FetchContent_Populate(webview)

set(SOURCES
    src/main.cpp
    src/Downloader.cpp
    src/Launcher.cpp
)

if(APPLE)
    enable_language(OBJCXX)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES})
    set_source_files_properties(src/main.cpp PROPERTIES LANGUAGE OBJCXX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${webview_SOURCE_DIR}/core/include
    ${httplib_SOURCE_DIR}
)

# Platform-specific linking
if(WIN32)
    # Fetch the WebView2 SDK (NuGet package). DOWNLOAD_NAME gives the .nupkg a
    # .zip extension so CMake extracts it; the headers webview.h needs live in
    # build/native/include. webview 0.12 has a built-in runtime loader, so only
    # the headers and system libraries are required (no WebView2Loader to link).
    FetchContent_Declare(
        webview2
        URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2792.45"
        DOWNLOAD_NAME webview2.zip
    )
    FetchContent_MakeAvailable(webview2)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${webview2_SOURCE_DIR}/build/native/include
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        advapi32 ole32 shell32 shlwapi user32 version
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE WEBVIEW_EDGE)

    # Hide the console window in release builds. WIN32_EXECUTABLE selects the
    # WINDOWS subsystem, so point the entry point back at the standard main().
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
        if(MSVC)
            target_link_options(${PROJECT_NAME} PRIVATE /ENTRY:mainCRTStartup)
        endif()
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
