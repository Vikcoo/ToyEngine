set(SHADERS
        graphics.vert
        graphics.frag
)

spirv_shaders(SHADER_HEADERS
        SPIRV_VERSION 1.0 # Currently targeting Vulkan 1.0
        SOURCES ${SHADERS}
)

add_executable(ToyEngine
        Main/Private/Main.cpp
        ${SHADER_HEADERS}
)

# 关键：链接 TELog 模块，声明依赖关系
# PRIVATE 表示 内部使用 TELog，不传递给依赖的模块
# 若其他模块依赖时也需要用 TELog，可改为 PUBLIC
target_link_libraries(ToyEngine PRIVATE TELog TEWindow TERHI)

add_subdirectory(Runtime)