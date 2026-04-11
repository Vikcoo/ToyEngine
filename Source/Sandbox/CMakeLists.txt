add_executable(Sandbox Main.cpp)

target_link_libraries(Sandbox PRIVATE Engine Asset Scene)

target_compile_definitions(Sandbox PRIVATE
    TE_PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
)

set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# OpenGL 阶段：Shader 源文件保留在 Content/Shaders/OpenGL/ 中，运行时直接读取
# CompiledShaders 目录预留给未来 Vulkan 的 SPIR-V 编译产物
# te_copy_shaders 不再需要调用
