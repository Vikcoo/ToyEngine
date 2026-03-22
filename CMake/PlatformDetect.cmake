# ToyEngine 平台检测
# 根据操作系统设置 TE_PLATFORM_XXX 变量
# 各模块通过 target_compile_definitions 将这些变量传递给 C++ 代码

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TE_PLATFORM_MACOS TRUE)
    set(TE_PLATFORM_NAME "macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TE_PLATFORM_WINDOWS TRUE)
    set(TE_PLATFORM_NAME "Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TE_PLATFORM_LINUX TRUE)
    set(TE_PLATFORM_NAME "Linux")
else()
    message(WARNING "Unknown platform: ${CMAKE_SYSTEM_NAME}")
    set(TE_PLATFORM_NAME "Unknown")
endif()

message(STATUS "[ToyEngine] Platform: ${TE_PLATFORM_NAME}")
