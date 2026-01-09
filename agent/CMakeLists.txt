cmake_minimum_required(VERSION 3.30)
project(DLP_Agent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Network REQUIRED)

add_executable(DLP_Agent main.cpp
        src/Logger.cpp
        src/ConfigManager.cpp
        src/NetworkManager.cpp
        src/PolicyChecker.cpp
        src/FileMonitor.cpp
        src/Agent.cpp
        src/ContentAnalyzer.cpp
        src/EventQueue.cpp
        include/Logger.h
        include/Agent.h
        include/ConfigManager.h
        include/NetworkManager.h
        include/PolicyChecker.h
        include/FileMonitor.h
        include/ContentAnalyzer.h
        include/EventQueue.h
)
target_link_libraries(DLP_Agent PRIVATE Qt6::Core Qt6::Network)
