project(Sandbox)

# 创建可执行文件
add_executable(Sandbox
    Main.cpp
)

# 链接Runtime模块
# Platform会自动传递Core依赖（因为Platform PUBLIC链接了Core）
target_link_libraries(Sandbox PRIVATE
    Platform
    RHI
)

# 设置工作目录为项目根目录（方便访问Content）
set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# 定义项目根目录路径（用于运行时查找资源）
# 将路径中的反斜杠转换为正斜杠，并转义引号
string(REPLACE "\\" "/" PROJECT_ROOT_NORMALIZED "${CMAKE_SOURCE_DIR}")
target_compile_definitions(Sandbox PRIVATE
    TE_PROJECT_ROOT_DIR="${PROJECT_ROOT_NORMALIZED}"
)

message(STATUS "Sandbox application configured")
message(STATUS "  Project root: ${CMAKE_SOURCE_DIR}")

