cmake_minimum_required(VERSION 3.18)
project(ToyEngine VERSION 0.1.0 LANGUAGES C CXX)

# 包含引擎构建选项（必须在 project() 之后）
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
include(EngineOptions)
include(PlatformDetect)
include(ShaderCompile)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(TE_BUILD_SANDBOX "Build Sandbox application" ON)
option(TE_BUILD_TESTS   "Build test executables"    ON)

add_subdirectory(ThirdParty)
add_subdirectory(Source)

if(TE_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
