project(Sandbox)

# 包含 Shader 编译脚本
include(${CMAKE_SOURCE_DIR}/CMake/CompileShaders.cmake)

# 编译 Shader
compile_shaders(
    OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Content/Shaders/Common"
    SHADERS
        "${CMAKE_SOURCE_DIR}/Content/Shaders/Common/triangle.vert"
        "${CMAKE_SOURCE_DIR}/Content/Shaders/Common/triangle.frag"
)

# 创建可执行文件
add_executable(Sandbox
    Main.cpp
    ${SHADER_OUTPUTS}  # 将编译后的 shader 添加到依赖中
)

# 链接Runtime模块
# Platform会自动传递Core依赖（因为Platform PUBLIC链接了Core）
target_link_libraries(Sandbox PRIVATE
    Platform
    RHI
	Asset
	Renderer
)

# 设置工作目录为项目根目录（方便访问Content）
set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

message(STATUS "Sandbox application configured")
message(STATUS "  Project root: ${CMAKE_SOURCE_DIR}")

