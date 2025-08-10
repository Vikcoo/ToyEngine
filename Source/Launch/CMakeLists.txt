# Launch目录的CMakeLists.txt

# 创建可执行文件
add_executable(ToyEngine 
    Private/ToyEngine.cpp 
    Public/ToyEngine.h
)

# 设置包含目录
target_include_directories(ToyEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

# 设置为启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ToyEngine)

# 关键：链接 TELog 模块，声明依赖关系
# PRIVATE 表示 内部使用 TELog，不传递给依赖的模块
# 若其他模块依赖时也需要用 TELog，可改为 PUBLIC
target_link_libraries(ToyEngine
    PRIVATE
        TELog
        TEWindow
)