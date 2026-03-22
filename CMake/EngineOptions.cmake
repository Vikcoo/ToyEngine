# ToyEngine 构建选项
# 此文件集中管理所有引擎构建选项

# RHI 后端选择（Phase 2+ 使用，Phase 1 仅占位）
option(TE_RHI_VULKAN "Build Vulkan RHI backend" OFF)
option(TE_RHI_OPENGL "Build OpenGL RHI backend" ON)
option(TE_RHI_D3D12 "Build D3D12 RHI backend" OFF)

# 应用选项（根 CMakeLists.txt 已定义，此处留作参考）
# option(TE_BUILD_SANDBOX "Build Sandbox application" ON)
# option(TE_BUILD_TESTS "Build test executables" ON)

# 验证：至少启用一个 RHI 后端时才构建渲染模块（Phase 2 时启用）
# set(TE_HAS_RHI_BACKEND ${TE_RHI_VULKAN} OR ${TE_RHI_OPENGL} OR ${TE_RHI_D3D12})
