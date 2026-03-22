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

# 依赖 Core（Math 类型）和 RHI（接口）
target_link_libraries(Renderer
    PUBLIC
        Core
        RHI
)
