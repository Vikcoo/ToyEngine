# CMakeList.txt: ToyEngine 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.10)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ToyEngine")


set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -Wall")

# 关键：为 MSVC 编译器添加 /utf-8 选项，启用 UTF-8 编码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    message("windows")
    add_definitions(-DTE_WIN32)
elseif(UNIX AND NOT APPLE)
    message("linux")
    add_definitions(-DTE_LINUX)
elseif(APPLE)
    message("macos")
    add_definitions(-DTE_MACOS)
endif()

option(USE_OPENGL "Use OpenGL as graphics API" OFF)
option(USE_VULKAN "Use Vulkan as graphics API" ON)
option(USE_DIRECTX "Use DirectX as graphics API" OFF)
if(USE_OPENGL)
    add_definitions(-DTE_OPENGL)
elseif(USE_VULKAN)
    add_definitions(-DTE_VULKAN)
elseif(USE_DIRECTX)
    add_definitions(-DTE_DIRECTX)
endif()


# 添加子目录
add_subdirectory(Source)
add_subdirectory(ThirdParty)

# 设置ToyEngine为启动项目（需要在所有目标定义之后）
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ToyEngine)
