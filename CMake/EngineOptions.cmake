# ToyEngine 构建选项
# 此文件集中管理所有引擎构建选项

# RHI 后端选择。Vulkan 选项当前用于启用 SPIR-V 前置构建；RHIVulkan 实现将在后续阶段接入。
option(TE_RHI_VULKAN "Enable Vulkan backend prerequisites (RHIVulkan is planned)" OFF)
option(TE_RHI_OPENGL "Build OpenGL RHI backend" ON)
option(TE_RHI_D3D12 "Build D3D12 RHI backend" OFF)

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

# 验证：至少启用一个 RHI 后端时才构建渲染模块（Phase 2 时启用）
# set(TE_HAS_RHI_BACKEND ${TE_RHI_VULKAN} OR ${TE_RHI_OPENGL} OR ${TE_RHI_D3D12})
