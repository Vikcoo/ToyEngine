cmake_minimum_required (VERSION 4.0.0)

project ("ToyEngine")

# 启用并行编译（数字为线程数，建议设为 CPU 核心数，如 8）
set(CMAKE_BUILD_PARALLEL_LEVEL 14)

# 为 Release模式设置添加的C/C++编译器添加额外编译选项
# 对 C 编译器
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -Wall -Wextra -Wpedantic -Werror")
# 对 C++ 编译器
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -Wall -Wextra -Wpedantic -Werror")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 资源目录
Set(TE_DEFINE_RES_ROOT_DIR "${CMAKE_SOURCE_DIR}/Content/")
add_definitions(-DTE_DEFINE_RES_ROOT_DIR="${TE_DEFINE_RES_ROOT_DIR}")
message("123${TE_DEFINE_RES_ROOT_DIR}")

if(WIN32)
    message("Target OS: windows")
    add_definitions(-DTE_WIN32)
elseif(UNIX AND NOT APPLE)
    message("Target OS: linux")
    add_definitions(-DTE_LINUX)
elseif(APPLE)
    message("Target OS: macos")
    add_definitions(-DTE_MACOS)
endif()

option(USE_OPENGL "Use OpenGL as graphics API" OFF)
option(USE_VULKAN "Use Vulkan as graphics API" ON)
option(USE_DIRECTX "Use DirectX as graphics API" OFF)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SPIR-V)
#
# 
if(USE_OPENGL)
    add_definitions(-DTE_OPENGL)
elseif(USE_VULKAN)
    add_definitions(-DTE_VULKAN)
elseif(USE_DIRECTX)
    add_definitions(-DTE_DIRECTX)
endif()
# 检查选项互斥性（确保只能选择一个 API）
set(GRAPHICS_API_COUNT 0)
if(USE_OPENGL)
    math(EXPR GRAPHICS_API_COUNT "${GRAPHICS_API_COUNT} + 1")
endif()
if(USE_VULKAN)
    math(EXPR GRAPHICS_API_COUNT "${GRAPHICS_API_COUNT} + 1")
endif()
if(USE_DIRECTX)
    math(EXPR GRAPHICS_API_COUNT "${GRAPHICS_API_COUNT} + 1")
endif()
# 如果选择了多个或零个API，抛出错误并提示
if(GRAPHICS_API_COUNT GREATER 1)
    message(FATAL_ERROR "Only one graphics API can be enabled at a time (OpenGL/Vulkan/DirectX)")
elseif(GRAPHICS_API_COUNT EQUAL 0)
    message(FATAL_ERROR "At least one graphics API must be enabled (OpenGL/Vulkan/DirectX)")
endif()


# 添加子目录
add_subdirectory(Source)
add_subdirectory(ThirdParty)