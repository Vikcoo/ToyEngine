# ToyEngine Renderer Module
# 渲染侧数据 + 渲染器（对应 UE5 的 Renderer 模块）

# Renderer 静态库
add_library(Renderer STATIC
    # 实现文件
    Private/FStaticMeshSceneProxy.cpp
    Private/FScene.cpp
    Private/SceneRenderer.cpp
)

# 头文件搜索路径
target_include_directories(Renderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 依赖 Core（Math 类型）、RHI（接口）和 Asset（TStaticMesh 类型）
target_link_libraries(Renderer
    PUBLIC
        Core
        RHI
        Asset
)

# 提供项目根目录宏，用于定位 Shader 文件等资源
target_compile_definitions(Renderer PRIVATE
    TE_PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
)
