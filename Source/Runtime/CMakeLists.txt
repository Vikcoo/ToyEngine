add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Input)

# RHI 相关模块：
# - OpenGLRHI 提供具体后端实现与后端装配入口
# - RHI 仅保留抽象接口，不反向依赖具体后端
if(TE_RHI_OPENGL)
    add_subdirectory(OpenGLRHI)
endif()
# if(TE_RHI_VULKAN)
#     add_subdirectory(VulkanRHI)
# endif()
# if(TE_RHI_D3D12)
#     add_subdirectory(D3D12RHI)
# endif()
add_subdirectory(RHI)

# Asset 模块：资产管理（TStaticMesh、FAssetImporter）
add_subdirectory(Asset)

# UE5 架构模块：Renderer（渲染侧数据 + 渲染器）和 Scene（游戏侧数据）
add_subdirectory(Renderer)
add_subdirectory(Scene)

add_subdirectory(Engine)
