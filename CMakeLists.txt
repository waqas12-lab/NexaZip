cmake_minimum_required(VERSION 3.20)
project(NexaZip VERSION 3.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
find_package(ZLIB REQUIRED)

qt_add_executable(NexaZip
    MACOSX_BUNDLE
    src/main.cpp
    src/MainWindow.cpp
    archive/ZipEngine.cpp
    include/MainWindow.h
    archive/ZipEngine.h
    resources/resources.qrc
)

target_include_directories(NexaZip PRIVATE include archive)
target_link_libraries(NexaZip PRIVATE Qt6::Widgets Qt6::Concurrent ZLIB::ZLIB)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(NexaZip PRIVATE -Wall -Wextra)
endif()

set_target_properties(NexaZip PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "NexaZip"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.waqas.nexazip"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
)
