# ToyEngine Scene Module
# 游戏侧数据（对应 UE5 的 Scene/World/Actor/Component 体系）

# Scene 静态库
add_library(Scene STATIC
    # 实现文件
    Private/SceneComponent.cpp
    Private/PrimitiveComponent.cpp
    Private/MeshComponent.cpp
    Private/CameraComponent.cpp
    Private/FlyCameraController.cpp
    Private/Actor.cpp
    Private/World.cpp
)

# 头文件搜索路径
target_include_directories(Scene
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 依赖 Core（Math/Transform）+ RHI（前向声明）+ Renderer（FScene/Proxy 类型）+ Asset（TStaticMesh）+ Input（TFlyCameraController）
target_link_libraries(Scene
    PUBLIC
        Core
        RHI
        Renderer
        Asset
        Input
)
