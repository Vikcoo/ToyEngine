# 平台检测脚本

# 检测操作系统
if(WIN32)
    message(STATUS "[PlatformDetection] Platform: Windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "[PlatformDetection] Platform: Linux")
elseif(APPLE)
    message(STATUS "[PlatformDetection] Platform: macOS")
else()
    message(FATAL_ERROR "[PlatformDetection] Unsupported platform!")
endif()

# 检测编译器
if(MSVC)
    message(STATUS "[CompilerDetection] Compiler: MSVC")
    # MSVC特定编译选项
    add_compile_options(/utf-8)     # 使用UTF-8编码（spdlog需要）
    add_compile_options(/W4)        # 设置警告级别
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "[CompilerDetection] Compiler: GCC")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "[CompilerDetection] Compiler: Clang")
endif()

# 检测构建类型
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "[CompilerDetection] Build Type: Debug")
    add_definitions(-DTE_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "[CompilerDetection] Build Type: Release")
    add_definitions(-DTE_RELEASE)
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message(STATUS "[CompilerDetection] Build Type: RelWithDebInfo")
    add_definitions(-DTE_RELEASE -DTE_DEBUG_INFO)
endif()

