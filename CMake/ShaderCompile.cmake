# ToyEngine Shader 编译管理
# OpenGL 继续在运行时编译 GLSL；启用 Vulkan 时额外生成 SPIR-V。
function(te_compile_shaders)
    cmake_parse_arguments(ARG "" "TARGET;SHADERS_DIR;OUTPUT_DIR" "SHADERS" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "te_compile_shaders: TARGET is required")
    endif()
    if(NOT ARG_SHADERS_DIR)
        message(FATAL_ERROR "te_compile_shaders: SHADERS_DIR is required")
    endif()
    if(NOT ARG_SHADERS)
        message(FATAL_ERROR "te_compile_shaders: SHADERS must list the runtime shader sources")
    endif()

    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/CompiledShaders/Vulkan")
    endif()

    if(NOT TE_RHI_VULKAN)
        message(STATUS "[Shader] OpenGL-only mode: SPIR-V compilation disabled")
        return()
    endif()

    find_program(TE_GLSLC_EXECUTABLE
        NAMES glslc glslc.exe
        HINTS
            "$ENV{VULKAN_SDK}/Bin"
            "$ENV{VULKAN_SDK}/bin"
    )
    if(NOT TE_GLSLC_EXECUTABLE)
        message(FATAL_ERROR
            "[Shader] TE_RHI_VULKAN=ON requires glslc. Install the Vulkan SDK and configure VULKAN_SDK.")
    endif()

    set(SPIRV_OUTPUTS)
    set(COMMON_BINDINGS "${ARG_SHADERS_DIR}/Common/RHIDescriptorBindings.glsl")
    foreach(SHADER_RELATIVE_PATH IN LISTS ARG_SHADERS)
        set(SHADER_FILE "${ARG_SHADERS_DIR}/${SHADER_RELATIVE_PATH}")
        if(NOT EXISTS "${SHADER_FILE}")
            message(FATAL_ERROR "[Shader] Shader source not found: ${SHADER_FILE}")
        endif()

        get_filename_component(SHADER_NAME "${SHADER_RELATIVE_PATH}" NAME)
        set(OUTPUT_FILE "${ARG_OUTPUT_DIR}/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT "${OUTPUT_FILE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ARG_OUTPUT_DIR}"
            COMMAND "${TE_GLSLC_EXECUTABLE}"
                    -DTE_RHI_VULKAN=1
                    --target-env=vulkan1.3
                    -I "${ARG_SHADERS_DIR}"
                    "${SHADER_FILE}"
                    -o "${OUTPUT_FILE}"
            DEPENDS "${SHADER_FILE}" "${COMMON_BINDINGS}"
            COMMENT "[Shader] Compiling SPIR-V: ${SHADER_NAME}"
            VERBATIM
        )
        list(APPEND SPIRV_OUTPUTS "${OUTPUT_FILE}")
    endforeach()

    add_custom_target(${ARG_TARGET} ALL DEPENDS ${SPIRV_OUTPUTS})
    message(STATUS "[Shader] Vulkan SPIR-V output: ${ARG_OUTPUT_DIR}")
endfunction()

# 保留旧函数名兼容性（已弃用）
function(te_copy_shaders)
    message(FATAL_ERROR "[Shader] te_copy_shaders has been removed. Use te_compile_shaders.")
endfunction()
