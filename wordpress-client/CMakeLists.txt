cmake_minimum_required(VERSION 3.20)
project(wp-desktop-client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia MultimediaWidgets)
find_package(CURL REQUIRED)

# nlohmann/json: prefer vendored copy from parent repo, else find installed
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../third-party/nlohmann-json/single_include/nlohmann/json.hpp")
    set(JSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third-party/nlohmann-json/single_include")
    message(STATUS "Using vendored nlohmann/json")
elseif(EXISTS "/usr/include/nlohmann/json.hpp")
    set(JSON_INCLUDE_DIR "/usr/include")
    message(STATUS "Using system nlohmann/json")
else()
    find_package(nlohmann_json REQUIRED)
    set(JSON_INCLUDE_DIR "")
endif()

# QtKeychain — optional, but strongly recommended for secure credential storage
find_package(Qt6Keychain QUIET)
if(NOT Qt6Keychain_FOUND)
    find_package(qtkeychain QUIET)
endif()

set(SOURCES
    src/main.cpp
    src/util/util-curl.cpp
    src/api/wp-auth.cpp
    src/api/wp-client.cpp
    src/ui/main-window.cpp
    src/ui/site-manager.cpp
    src/ui/post-list.cpp
    src/ui/post-editor.cpp
    src/ui/media-picker.cpp
    src/ui/camera-capture.cpp
)

# ---- Platform-specific ----
if(ANDROID)
    # Qt for Android: package as APK/AAB
    set_target_properties(wp-desktop-client PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_VERSION_NAME       "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE       "1"
        QT_ANDROID_MIN_SDK_VERSION    "26"
        QT_ANDROID_TARGET_SDK_VERSION "34"
    )
elseif(IOS)
    set_target_properties(wp-desktop-client PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone + iPad
    )
endif()

add_executable(wp-desktop-client ${SOURCES})

target_include_directories(wp-desktop-client PRIVATE
    src
    ${JSON_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(wp-desktop-client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${CURL_LIBRARIES}
)

if(TARGET Qt6Keychain::Qt6Keychain)
    target_link_libraries(wp-desktop-client PRIVATE Qt6Keychain::Qt6Keychain)
    target_compile_definitions(wp-desktop-client PRIVATE HAS_QTKEYCHAIN)
    message(STATUS "QtKeychain found — credentials stored in system keychain")
elseif(TARGET qtkeychain::Qt6Keychain)
    target_link_libraries(wp-desktop-client PRIVATE qtkeychain::Qt6Keychain)
    target_compile_definitions(wp-desktop-client PRIVATE HAS_QTKEYCHAIN)
    message(STATUS "QtKeychain found — credentials stored in system keychain")
else()
    message(WARNING "QtKeychain NOT found — credentials will be stored insecurely. Install qt6keychain for secure storage.")
endif()

if(MSVC)
    target_compile_options(wp-desktop-client PRIVATE /W4 /WX- /EHa /MP)
else()
    target_compile_options(wp-desktop-client PRIVATE -Wall -Wextra -Wpedantic)
endif()
