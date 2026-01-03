cmake_minimum_required(VERSION 3.20)
project(ToyEngine VERSION 0.1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC编译器选项
if(MSVC)
    # 使用UTF-8编码（spdlog需要）
    add_compile_options(/utf-8)
    # 设置警告级别
    add_compile_options(/W4)
endif()

# 包含CMake辅助脚本
include(CMake/PlatformDetection.cmake)
include(CMake/EngineOptions.cmake)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 第三方库
add_subdirectory(ThirdParty)

# 引擎源码
add_subdirectory(Source)

# 定义项目根目录路径（用于运行时查找资源）
# 将路径中的反斜杠转换为正斜杠，并转义引号
string(REPLACE "\\" "/" PROJECT_ROOT_NORMALIZED "${CMAKE_SOURCE_DIR}")
target_compile_definitions(Sandbox PRIVATE
    TE_PROJECT_ROOT_DIR="${PROJECT_ROOT_NORMALIZED}/"
)

