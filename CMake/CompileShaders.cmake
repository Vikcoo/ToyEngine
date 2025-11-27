# Shader编译辅助脚本

# 查找glslc编译器（Vulkan SDK）
find_program(GLSLC_EXECUTABLE
    NAMES glslc
    HINTS
        $ENV{VULKAN_SDK}/Bin
        $ENV{VULKAN_SDK}/bin
    REQUIRED
)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK.")
endif()

message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")

# 编译Shader的函数
# compile_shaders(
#     OUTPUT_DIR "path/to/output"
#     SHADERS "shader1.vert" "shader1.frag" ...
# )
function(compile_shaders)
    cmake_parse_arguments(
        SHADER
        ""
        "OUTPUT_DIR"
        "SHADERS"
        ${ARGN}
    )
    
    foreach(SHADER_FILE ${SHADER_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
        set(OUTPUT_FILE "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER_FILE} -o ${OUTPUT_FILE}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling shader: ${SHADER_NAME}"
        )
        
        list(APPEND SHADER_OUTPUTS ${OUTPUT_FILE})
    endforeach()
    
    set(SHADER_OUTPUTS ${SHADER_OUTPUTS} PARENT_SCOPE)
endfunction()

