cmake_minimum_required(VERSION 3.20)
project(wp-desktop-client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt modules — Network replaces libcurl; works on all platforms including Android/iOS
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia MultimediaWidgets)

# nlohmann/json — vendored → system → FetchContent (works on all platforms)
include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../third-party/nlohmann-json/single_include/nlohmann/json.hpp")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/../third-party/nlohmann-json/single_include")
    message(STATUS "nlohmann/json: vendored copy")
else()
    find_package(nlohmann_json QUIET)
    if(NOT nlohmann_json_FOUND)
        message(STATUS "nlohmann/json: downloading via FetchContent")
        FetchContent_Declare(nlohmann_json
            URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        )
        set(JSON_BuildTests OFF CACHE INTERNAL "")
        FetchContent_MakeAvailable(nlohmann_json)
    else()
        message(STATUS "nlohmann/json: system package")
    endif()
endif()

# QtKeychain — optional, strongly recommended
find_package(Qt6Keychain QUIET)
if(NOT Qt6Keychain_FOUND)
    find_package(qtkeychain QUIET)
endif()

set(SOURCES
    src/main.cpp
    src/api/wp-auth.cpp
    src/api/wp-client.cpp
    src/ui/main-window.cpp
    src/ui/site-manager.cpp
    src/ui/post-list.cpp
    src/ui/post-editor.cpp
    src/ui/media-picker.cpp
    src/ui/camera-capture.cpp
)

qt_add_executable(wp-desktop-client ${SOURCES})

# ---- macOS bundle ----
if(APPLE AND NOT IOS)
    set_target_properties(wp-desktop-client PROPERTIES
        MACOSX_BUNDLE                   TRUE
        MACOSX_BUNDLE_BUNDLE_NAME       "WordPress Desktop"
        MACOSX_BUNDLE_BUNDLE_VERSION    "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER    "com.wpdesktop.client"
    )
endif()

# ---- Android APK ----
if(ANDROID)
    set_target_properties(wp-desktop-client PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_VERSION_NAME         "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE         "1"
        QT_ANDROID_MIN_SDK_VERSION      "26"
        QT_ANDROID_TARGET_SDK_VERSION   "34"
    )
endif()

# ---- iOS bundle ----
if(IOS)
    set_target_properties(wp-desktop-client PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_SKIP_INSTALL       NO
    )
endif()

target_include_directories(wp-desktop-client PRIVATE src)

target_link_libraries(wp-desktop-client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    nlohmann_json
)

if(TARGET Qt6Keychain::Qt6Keychain)
    target_link_libraries(wp-desktop-client PRIVATE Qt6Keychain::Qt6Keychain)
    target_compile_definitions(wp-desktop-client PRIVATE HAS_QTKEYCHAIN)
    message(STATUS "QtKeychain: system keychain enabled")
elseif(TARGET qtkeychain::Qt6Keychain)
    target_link_libraries(wp-desktop-client PRIVATE qtkeychain::Qt6Keychain)
    target_compile_definitions(wp-desktop-client PRIVATE HAS_QTKEYCHAIN)
    message(STATUS "QtKeychain: system keychain enabled")
else()
    message(WARNING "QtKeychain NOT found — install qt6keychain for secure credential storage.")
endif()

if(MSVC)
    target_compile_options(wp-desktop-client PRIVATE /W4 /WX- /EHa /MP)
else()
    target_compile_options(wp-desktop-client PRIVATE -Wall -Wextra -Wpedantic)
endif()
