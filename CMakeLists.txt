cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 20)
project(raw_renderer VERSION 0.0.1 LANGUAGES CXX)

file(GLOB SOURCES
    "raw_renderer/entry_point.cpp"
    "raw_renderer/src/*.cpp", 
    "raw_renderer/src/*/*.cpp", 
    "raw_renderer/src/*/*/*.cpp", 
    "raw_renderer/src/*/*/*/*.cpp" )

add_executable(${PROJECT_NAME} ${SOURCES} )
