add_executable(Sandbox Main.cpp)

target_link_libraries(Sandbox PRIVATE Engine)

target_compile_definitions(Sandbox PRIVATE
    TE_ENABLE_GLOBAL_NEW_DELETE=1
    TE_PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}/"
)

set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
