# ToyEngine Renderer Module
# 渲染侧数据 + 渲染器（对应 UE5 的 Renderer 模块）
# 提供 IRenderSceneBridge 的默认实现，负责接收游戏侧同步请求

# Renderer 静态库
add_library(Renderer STATIC
    # 实现文件
    Private/FStaticMeshSceneProxy.cpp
    Private/FScene.cpp
    Private/RenderSceneBridge.cpp
    Private/SceneRenderer.cpp
)

# 头文件搜索路径
target_include_directories(Renderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 依赖 Core（Math 类型）、RHI（接口）、Scene（桥接接口）和 Asset（TStaticMesh 类型）
target_link_libraries(Renderer
    PUBLIC
        Core
        RHI
        Scene
        Asset
)

# 提供项目根目录宏，用于定位 Shader 文件等资源
target_compile_definitions(Renderer PRIVATE
    TE_PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
)
