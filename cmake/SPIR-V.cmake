find_program(GLSLANGVALIDATOR_COMMAND glslangValidator)
if (NOT GLSLANGVALIDATOR_COMMAND)
    message("no glslangValidator!!")
endif ()

function(spirv_shaders ret)
    set(options)
    set(oneValueArgs SPIRV_VERSION)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(_spirvshaders "${options}" "${onevalueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT _spirvshaders_SPIRV_VERSION)
        set(_spirvshaders_SPIRV_VERSION 1.0)
    endif ()

    # 检查源文件是否存在


    foreach(GLSL ${_spirvshaders_SOURCES})
        string(MAKE_C_IDENTIFIER ${GLSL} IDENTIFIER)
        set(HEADER "${TE_DEFINE_RES_ROOT_DIR}Shaders/${GLSL}.spv")
        set(GLSL "${TE_DEFINE_RES_ROOT_DIR}Shaders/${GLSL}")
        if(NOT EXISTS "${GLSL}")
            message(FATAL_ERROR "Missing GLSL source file: ${GLSL}")
        endif()
        message("GLSL Command: ${GLSLANGVALIDATOR_COMMAND} -V --target-env spirv${_spirvshaders_SPIRV_VERSION} ${GLSL} -o ${HEADER}")
        add_custom_command(
                OUTPUT ${HEADER}
                COMMAND ${GLSLANGVALIDATOR_COMMAND} -V --target-env spirv${_spirvshaders_SPIRV_VERSION} ${GLSL} -o ${HEADER}
                DEPENDS ${GLSL})
        list(APPEND HEADERS ${HEADER})
    endforeach()
    set(${ret} "${HEADERS}" PARENT_SCOPE)
endfunction(spirv_shaders)