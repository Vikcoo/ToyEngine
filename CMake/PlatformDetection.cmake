# 平台检测脚本

# 检测操作系统
if(WIN32)
    message(STATUS "Platform: Windows")
    add_definitions(-DTE_PLATFORM_WINDOWS)
    set(TE_PLATFORM_NAME "Windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Platform: Linux")
    add_definitions(-DTE_PLATFORM_LINUX)
    set(TE_PLATFORM_NAME "Linux")
elseif(APPLE)
    message(STATUS "Platform: macOS")
    add_definitions(-DTE_PLATFORM_MACOS)
    set(TE_PLATFORM_NAME "macOS")
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

# 检测编译器
if(MSVC)
    message(STATUS "Compiler: MSVC")
    add_definitions(-DTE_COMPILER_MSVC)
    # MSVC特定编译选项
    add_compile_options(/W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Compiler: GCC")
    add_definitions(-DTE_COMPILER_GCC)
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Compiler: Clang")
    add_definitions(-DTE_COMPILER_CLANG)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 检测构建类型
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Build Type: Debug")
    add_definitions(-DTE_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Build Type: Release")
    add_definitions(-DTE_RELEASE)
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message(STATUS "Build Type: RelWithDebInfo")
    add_definitions(-DTE_RELEASE -DTE_DEBUG_INFO)
endif()

