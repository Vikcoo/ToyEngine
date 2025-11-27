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

message(STATUS "Sandbox application configured")

