add_executable(Sandbox Main.cpp)

target_link_libraries(Sandbox PRIVATE Engine Asset World)

target_compile_definitions(Sandbox PRIVATE
    TE_PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
)

set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Shader 编译入口由根 CMakeLists.txt 统一装配；Sandbox 不直接管理后端产物。
