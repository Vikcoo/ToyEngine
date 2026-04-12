# ToyEngine Asset Module
# 资产管理模块（对应 UE5 的 Asset / AssetRegistry 子系统）
#
# 设计原则：
# - Assimp 头文件仅在 PRIVATE 中链接，不泄漏到依赖 Asset 的其他模块
# - Public 接口只暴露引擎自有类型（TStaticMesh、FAssetImporter）

add_library(Asset STATIC
    Private/TStaticMesh.cpp
    Private/AssetImporter.cpp
)

target_include_directories(Asset
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# PUBLIC 依赖 Core：TStaticMesh 使用 MathTypes（Vector3 等）
# PRIVATE 依赖 assimp：Assimp 头文件仅在 .cpp 中引用，不泄漏
target_link_libraries(Asset
    PUBLIC  Core
    PRIVATE assimp
)
