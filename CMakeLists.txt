cmake_minimum_required(VERSION 3.16)
project(SerialTool VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort)

set(SOURCES
    src/main.cpp
    src/serialworker.cpp
    src/protocolparser.cpp
    src/ui/mainwindow.cpp
    src/ui/serialconfig.cpp
    src/ui/waveformwidget.cpp
)

set(HEADERS
    src/serialworker.h
    src/protocolparser.h
    src/ui/mainwindow.h
    src/ui/serialconfig.h
    src/ui/waveformwidget.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
)
