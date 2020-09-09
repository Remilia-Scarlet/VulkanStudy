cmake_minimum_required(VERSION 3.18.2)

set (PROJ_NAME "LearningVulkan")
project(${PROJ_NAME})

message(STATUS "Finding vulkan SDK...")

# 通过CMake的vulkan模块查找vulkan sdk位置
# 这里会自动返回一个Vulkan_FOUND，（ON 或 OFF)
# 头文件路径 - Vulkan_INCLUDE_DIRS
# lib路径 - Vulkan_LIBRARIES
find_package(Vulkan REQUIRED)

message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")

# Let's make a library and link vulkan
include_directories(${Vulkan_INCLUDE_DIRS})
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

# 搜集全部文件列表
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file (GLOB_RECURSE PROJ_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.*)
file (GLOB_RECURSE PROJ_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
add_executable(${PROJ_NAME} ${PROJ_FILES} ${PROJ_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJ_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJ_HEADERS})

# link vulkan lib
target_link_libraries(${PROJ_NAME} ${Vulkan_LIBRARIES})

# 输出文件夹
set_target_properties(${PROJ_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/binaries)