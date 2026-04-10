# ToyEngine RHI 抽象接口层
# 此模块仅定义图形 API 无关的抽象接口
# 不直接依赖任何图形 API（OpenGL/Vulkan/D3D12）

file(GLOB_RECURSE RHI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

add_library(RHI STATIC ${RHI_SOURCES})

target_include_directories(RHI
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# RHI 仅依赖 Core（日志、类型等），不依赖任何图形 API
target_link_libraries(RHI PUBLIC Core)
