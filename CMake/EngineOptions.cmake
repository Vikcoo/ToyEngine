# ToyEngine 构建选项
# 此文件集中管理所有引擎构建选项

# RHI 后端选择。当前可运行后端必须且只能选择一个。
option(TE_RHI_VULKAN "Build Vulkan RHI backend" OFF)
option(TE_RHI_OPENGL "Build OpenGL RHI backend" ON)
option(TE_RHI_D3D12 "Build D3D12 RHI backend" OFF)
option(TE_VULKAN_VALIDATION "Enable Vulkan validation layer" ON)

if(TE_RHI_D3D12)
    message(FATAL_ERROR
        "[ToyEngine] TE_RHI_D3D12 is reserved but not implemented. "
        "Select TE_RHI_OPENGL or TE_RHI_VULKAN.")
endif()

set(TE_RHI_BACKEND_COUNT 0)
foreach(TE_RHI_OPTION TE_RHI_VULKAN TE_RHI_OPENGL TE_RHI_D3D12)
    if(${TE_RHI_OPTION})
        math(EXPR TE_RHI_BACKEND_COUNT "${TE_RHI_BACKEND_COUNT} + 1")
    endif()
endforeach()

if(NOT TE_RHI_BACKEND_COUNT EQUAL 1)
    message(FATAL_ERROR
        "[ToyEngine] Exactly one RHI backend must be enabled. "
        "Set one of TE_RHI_OPENGL/TE_RHI_VULKAN/TE_RHI_D3D12 to ON and the others to OFF.")
endif()

# MinGW 可执行文件默认静态链接编译器运行库，确保开发产物无需额外部署
# libgcc_s_seh-1.dll、libstdc++-6.dll 与 libwinpthread-1.dll 即可直接启动。
option(TE_STATIC_MINGW_RUNTIME "Statically link MinGW runtime libraries into executables" ON)

if(MINGW AND TE_STATIC_MINGW_RUNTIME)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -static -static-libgcc -static-libstdc++")
    message(STATUS "[ToyEngine] MinGW executable runtime: static")
elseif(MINGW)
    message(STATUS "[ToyEngine] MinGW executable runtime: dynamic")
endif()

# 应用选项（根 CMakeLists.txt 已定义，此处留作参考）
# option(TE_BUILD_SANDBOX "Build Sandbox application" ON)
# option(TE_BUILD_TESTS "Build test executables" ON)
