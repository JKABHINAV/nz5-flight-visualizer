cmake_minimum_required(VERSION 3.16)
project(FlightVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Sql)

add_executable(FlightVisualizer
    src/main.cpp
    src/FlightData.h
    src/FlightTableModel.h
    src/MBTilesLoader.h
    src/Map2DWidget.h
    src/Globe3DWidget.h
    src/MainWindow.h
)

target_link_libraries(FlightVisualizer PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Sql
    opengl32
)