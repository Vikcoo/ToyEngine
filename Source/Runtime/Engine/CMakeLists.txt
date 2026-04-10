file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

add_library(Engine STATIC ${ENGINE_SOURCES})

target_include_directories(Engine
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# Engine 通过 RHI 抽象层操作 GPU，通过 Scene/Renderer 模块实现 UE5 架构
# Asset 模块提供 TStaticMesh 和 FAssetImporter
target_link_libraries(Engine PUBLIC Platform RHI Scene Renderer Asset Input)

# 具体 RHI 后端由外层装配（Engine）选择，避免 RHI 抽象层反向依赖后端实现
if(TE_RHI_OPENGL)
    target_compile_definitions(Engine PRIVATE TE_RHI_BACKEND_OPENGL=1)
    target_link_libraries(Engine PRIVATE OpenGLRHI)
endif()

# 提供项目根目录宏，用于定位 Shader 文件等资源
target_compile_definitions(Engine PRIVATE
    TE_PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
)
