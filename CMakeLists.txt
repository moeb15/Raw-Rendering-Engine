cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 20)
project(raw_renderer VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(external/SDL)

file(GLOB SOURCES
    "raw_renderer/entry_point.cpp"
    "raw_renderer/src/*.cpp", 
    "raw_renderer/src/*/*.cpp", 
    "raw_renderer/src/*/*/*.cpp", 
    "raw_renderer/src/*/*/*/*.cpp" )

add_executable(${PROJECT_NAME} ${SOURCES} )

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/raw_renderer/include>
    PRIVATE
        SDL3
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC

    PRIVATE
        SDL3::SDL3
)

set(LOGS_DIR ${CMAKE_BINARY_DIR}/logs)
target_compile_definitions(${PROJECT_NAME} PRIVATE LOG_FILE_DIR="${LOGS_DIR}")
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND if not exist ${LOGS_DIR} mkdir ${LOGS_DIR})
else()
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND mkdir -p ${LOGS_DIR})
endif()