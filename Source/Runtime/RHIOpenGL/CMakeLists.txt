# ToyEngine RHIOpenGL 后端
# OpenGL 实现层：依赖 RHI 抽象接口 + glad + OpenGL

file(GLOB_RECURSE RHIOPENGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

add_library(RHIOpenGL STATIC ${RHIOPENGL_SOURCES})

# 查找系统 OpenGL
find_package(OpenGL REQUIRED)

target_include_directories(RHIOpenGL
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# RHIOpenGL 需要 RHI 的头文件和 Core（通过 RHI 传递）
# 使用 PRIVATE 避免 RHI<->RHIOpenGL 的循环 PUBLIC 依赖
target_link_libraries(RHIOpenGL
    PRIVATE RHI glad OpenGL::GL
)
