cmake_minimum_required(VERSION 3.30)
project(DLP_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Widgets Network Charts Concurrent REQUIRED)

add_executable(DLP_GUI main.cpp
        src/MainWindow.cpp
        src/ApiClient.cpp
        src/DataModels.cpp
        src/Dialogs.cpp
        include/MainWindow.h
        include/ApiClient.h
        include/DataModels.h
        include/Dialogs.h
)
target_link_libraries(DLP_GUI PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
        Qt6::Charts
        Qt6::Concurrent
)