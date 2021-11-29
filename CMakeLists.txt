cmake_minimum_required(VERSION 3.5)

project(master_labeling)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
find_package(Qt5 COMPONENTS Widgets Core Network REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
    config.h
    mainwindow.ui
    pugixml.cpp
    rubbish_bin.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Core
    Qt5::Network
    stdc++fs
)

configure_file(config.h.in config.h @ONLY)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    ${PROJECT_BINARY_DIR}
)
