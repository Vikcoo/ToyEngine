add_library(TERHI STATIC
        Private/Vulkan/TEVKGraphicContext.cpp
        Public/Vulkan/TEVKGraphicContext.h
        Public/TEGraphicContext.h
        Private/TEGraphicContext.cpp
        Public/TEVKCommon.h
        Public/Vulkan/TEVKLogicDevice.h
        Private/Vulkan/TEVKLogicDevice.cpp
        Public/Vulkan/TEVKQueue.h
        Private/Vulkan/TEVKQueue.cpp
        Private/Vulkan/TEVKSwapChain.cpp
        Public/Vulkan/TEVKSwapChain.h
        Private/Vulkan/TEVKImageView.cpp
        Public/Vulkan/TEVKImageView.h
        Private/Vulkan/TEVKFrameBuffer.cpp
        Public/Vulkan/TEVKFrameBuffer.h
        Private/Vulkan/TEVKRenderPass.cpp
        Public/Vulkan/TEVKRenderPass.h
        Private/Vulkan/TEVKPipeline.cpp
        Public/Vulkan/TEVKPipeline.h
        Public/TEFileUtil.h
        Public/Vulkan/TEVKCommandBuffer.h
        Private/Vulkan/TEVKCommandBuffer.cpp
)

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message("----> find vulkan success: " ${Vulkan_INCLUDE_DIRS})
endif()

target_include_directories(TERHI
        PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>  # 对外暴露 Public 目录
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan  # RHI 内部私有头文件路径
)


target_link_libraries(TERHI PRIVATE TELog TEWindow glfw ${Vulkan_LIBRARY})

