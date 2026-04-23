# ToyEngine World Module
# 游戏侧数据（对应 UE5 的 World/Actor/Component 体系）
# 通过 IRenderScene 与渲染侧解耦

# World 静态库
add_library(World STATIC
    # 实现文件
    Private/SceneComponent.cpp
    Private/PrimitiveComponent.cpp
    Private/MeshComponent.cpp
    Private/LightComponent.cpp
    Private/CameraComponent.cpp
    Private/FlyCameraController.cpp
    Private/Actor.cpp
    Private/World.cpp
)

# 头文件搜索路径
target_include_directories(World
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 依赖 Core（Math/Transform）+ Asset（TStaticMesh）+ Input（TFlyCameraController）
# 不再直接依赖 Renderer，实现游戏侧与渲染侧边界解耦
target_link_libraries(World
    PUBLIC
        Core
        Asset
        Input
        RenderCore
)
