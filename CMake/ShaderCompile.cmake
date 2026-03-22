# ToyEngine Shader 编译管理
#
# 当前阶段（OpenGL）：
#   Shader 由运行时 glCompileShader 编译，源文件保留在 Content/Shaders/OpenGL/
#   CompiledShaders/ 目录预留给编译产物，当前阶段不使用
#
# 未来阶段（Vulkan）：
#   此模块将扩展为调用 glslangValidator / glslc 将 GLSL 编译为 SPIR-V
#   编译产物输出到 Content/Shaders/CompiledShaders/

# te_compile_shaders(TARGET target_name SHADERS_DIR source_shader_directory)
# 当前为占位函数，未来实现 SPIR-V 离线编译
function(te_compile_shaders)
    cmake_parse_arguments(ARG "" "TARGET;SHADERS_DIR;OUTPUT_DIR" "" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "te_compile_shaders: TARGET is required")
    endif()
    if(NOT ARG_SHADERS_DIR)
        message(FATAL_ERROR "te_compile_shaders: SHADERS_DIR is required")
    endif()

    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Content/Shaders/CompiledShaders")
    endif()

    # 未来实现示例（Vulkan SPIR-V 编译）：
    # find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin)
    # if(NOT GLSLC)
    #     message(FATAL_ERROR "glslc not found. Install Vulkan SDK.")
    # endif()
    #
    # file(GLOB_RECURSE SHADER_FILES
    #     "${ARG_SHADERS_DIR}/*.vert"
    #     "${ARG_SHADERS_DIR}/*.frag"
    #     "${ARG_SHADERS_DIR}/*.comp"
    # )
    #
    # foreach(SHADER_FILE ${SHADER_FILES})
    #     get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    #     set(OUTPUT_FILE "${ARG_OUTPUT_DIR}/${SHADER_NAME}.spv")
    #
    #     add_custom_command(
    #         OUTPUT ${OUTPUT_FILE}
    #         COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUTPUT_DIR}
    #         COMMAND ${GLSLC} ${SHADER_FILE} -o ${OUTPUT_FILE}
    #         DEPENDS ${SHADER_FILE}
    #         COMMENT "Compiling SPIR-V: ${SHADER_NAME}"
    #     )
    #     list(APPEND SPIRV_OUTPUTS ${OUTPUT_FILE})
    # endforeach()
    #
    # if(SPIRV_OUTPUTS)
    #     add_custom_target(${ARG_TARGET}_shaders ALL DEPENDS ${SPIRV_OUTPUTS})
    #     add_dependencies(${ARG_TARGET} ${ARG_TARGET}_shaders)
    # endif()

    message(STATUS "[Shader] OpenGL mode: shaders compiled at runtime, skipping offline compilation")
endfunction()

# 保留旧函数名兼容性（已弃用）
function(te_copy_shaders)
    message(STATUS "[Shader] te_copy_shaders is deprecated. Use te_compile_shaders for Vulkan SPIR-V compilation.")
endfunction()
