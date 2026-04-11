add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Input)

# RHI 相关模块：
# - RHIOpenGL 提供具体后端实现与后端装配入口
# - RHI 仅保留抽象接口，不反向依赖具体后端
if(TE_RHI_OPENGL)
    add_subdirectory(RHIOpenGL)
endif()
# if(TE_RHI_VULKAN)
#     add_subdirectory(RHIVulkan)
# endif()
# if(TE_RHI_D3D12)
#     add_subdirectory(RHID3D12)
# endif()
add_subdirectory(RHI)

# Asset 模块：资产管理（TStaticMesh、FAssetImporter）
add_subdirectory(Asset)

# UE5 风格运行时渲染结构：
# - RenderCore：SceneProxy / DrawCommand / RenderData 等共享渲染类型
# - World：游戏侧对象与 CreateSceneProxy 入口
# - Renderer：FScene 容器与实际渲染提交
add_subdirectory(RenderCore)
add_subdirectory(World)
add_subdirectory(Renderer)

add_subdirectory(Engine)
