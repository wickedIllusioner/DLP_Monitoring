cmake_minimum_required(VERSION 3.30)
project(DLP_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#include(FetchContent)
#FetchContent_Declare(Qlementine
#        GIT_REPOSITORY "https://github.com/oclero/qlementine.git"
#)
#FetchContent_MakeAvailable(Qlementine)

find_package(Qt6 COMPONENTS Core Widgets Network Charts Concurrent REQUIRED)

add_executable(DLP_GUI main.cpp
        # Основные компоненты
        src/MainWindow.cpp
        src/ApiClient.cpp
        src/DataModels.cpp
        src/Dialogs.cpp
        src/AgentInfoDialog.cpp
        src/AgentCreationDialog.cpp
        include/MainWindow.h
        include/ApiClient.h
        include/DataModels.h
        include/Dialogs.h
        include/AgentCreationDialog.h
        include/AgentInfoDialog.h

        # Виджеты
        widgets/src/ChartWidget.cpp
        widgets/include/ChartWidget.h
        widgets/src/StatisticsTabWidget.cpp
        widgets/include/StatisticsTabWidget.h
        widgets/include/StatsData.h
        widgets/include/PieChartWidget.h
        widgets/src/PieChartWidget.cpp
        widgets/include/BarChartWidget.h
        widgets/src/BarChartWidget.cpp
        widgets/include/StatsSummaryWidget.h
        widgets/src/StatsSummaryWidget.cpp
)
target_link_libraries(DLP_GUI PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
        Qt6::Charts
        Qt6::Concurrent
#        qlementine
)
