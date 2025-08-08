# Launch目录的CMakeLists.txt

# 创建可执行文件
add_executable(ToyEngine 
    Private/ToyEngine.cpp 
    Public/ToyEngine.h
)

# 设置包含目录
target_include_directories(ToyEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 设置目标属性
set_target_properties(ToyEngine PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# 设置为启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ToyEngine)
