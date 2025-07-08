cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 20)
project(raw_renderer VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(external/SDL)
add_subdirectory(external/glm)
add_subdirectory(external/vma)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(external/tinygltf)

find_package(Vulkan REQUIRED)

file(GLOB SOURCES
    "raw_renderer/src/*.cpp", 
    "raw_renderer/src/*/*.cpp", 
    "raw_renderer/src/*/*/*.cpp", 
    "raw_renderer/src/*/*/*/*.cpp",
    "external/imgui/imconfig.h",
    "external/imgui/imgui.h",
    "external/imgui/imgui.cpp",
    "external/imgui/imgui_draw.cpp",
    "external/imgui/imgui_internal.h",
    "external/imgui/imgui_tables.cpp",
    "external/imgui/imgui_widgets.cpp",
    "external/imgui/imstb_rectpack.h",
    "external/imgui/imstb_textedit.h",
    "external/imgui/imstb_truetype.h",
    "external/imgui/imgui_demo.cpp" )

add_executable(${PROJECT_NAME} ${SOURCES} )

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/raw_renderer/include>
        glm
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        SDL3
        VulkanMemoryAllocator
        "external/imgui"
        "external/wyhash"
        "external/tinygltf"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        glm
    PRIVATE
        ${Vulkan_LIBRARIES}
        SDL3::SDL3
        VulkanMemoryAllocator
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

if(WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/build/external/SDL/Debug/SDL3.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()

set(VULKAN_SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan)
set(VULKAN_SHADERS_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders-bin/vulkan)
target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_SHADERS_DIR="${VULKAN_SHADERS_BIN_DIR}")
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        _CRT_SECURE_NO_WARNINGS=1
        WIN32_LEAN_AND_MEAN
        NOMINMAX)
endif()

# Vulkan Shader Compilation
file (GLOB VULKAN_SHADERS 
    "${VULKAN_SHADERS_DIR}/*.frag" 
    "${VULKAN_SHADERS_DIR}/*.vert" 
    "${VULKAN_SHADERS_DIR}/*.comp")

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND if not exist ${VULKAN_SHADERS_BIN_DIR} mkdir ${VULKAN_SHADERS_BIN_DIR})
else()
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND mkdir -p ${VULKAN_SHADERS_BIN_DIR})
endif()

foreach(shader ${VULKAN_SHADERS})
    get_filename_component(SHADER_NAME ${shader} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND glslc -c ${shader} -o ${VULKAN_SHADERS_BIN_DIR}/${SHADER_NAME}.spv)
endforeach()