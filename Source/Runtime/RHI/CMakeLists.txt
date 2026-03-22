# ToyEngine RHI 抽象接口层
# 此模块仅定义图形 API 无关的抽象接口和工厂方法
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

# 根据启用的 RHI 后端定义宏，供 RHIFactory.cpp 使用
if(TE_RHI_OPENGL)
    target_compile_definitions(RHI PRIVATE TE_RHI_BACKEND_OPENGL=1)
    # RHIFactory.cpp 需要包含 OpenGLDevice.h（位于 OpenGLRHI/Private/）
    target_link_libraries(RHI PRIVATE OpenGLRHI)
    target_include_directories(RHI PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../OpenGLRHI/Private
    )
endif()

# 未来扩展：
# if(TE_RHI_VULKAN)
#     target_compile_definitions(RHI PRIVATE TE_RHI_BACKEND_VULKAN=1)
#     target_link_libraries(RHI PRIVATE VulkanRHI)
# endif()
# if(TE_RHI_D3D12)
#     target_compile_definitions(RHI PRIVATE TE_RHI_BACKEND_D3D12=1)
#     target_link_libraries(RHI PRIVATE D3D12RHI)
# endif()
