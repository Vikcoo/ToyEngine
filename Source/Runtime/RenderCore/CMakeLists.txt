# ToyEngine RenderCore Module
# 渲染侧共享数据结构：SceneProxy、MeshDrawCommand、资产级 RenderData

add_library(RenderCore STATIC
    Private/FStaticMeshRenderData.cpp
    Private/FStaticMeshSceneProxy.cpp
)

target_include_directories(RenderCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(RenderCore
    PUBLIC
        Core
        RHI
        Asset
)
