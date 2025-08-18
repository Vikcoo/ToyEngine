add_library(TEWindow
    # 公开
    Public/TEWindow.h
    Public/TEGLFWWindow.h
    Public/TEGraphicContext.h
    Public/TEVKGraphicContext.h
    Public/TEVKCommon.h
    Public/TEVKLogicDevice.h
    Public/TEVKQueue.h
    Public/TEVKSwapChain.h
    Public/TEVKRenderPass.h
    Public/TEVKFrameBuffer.h
    Public/TEVKImageView.h
    # 私有
    Private/TEWindow.cpp
    Private/TEGLFWWindow.cpp 
    Private/TEGraphicContext.cpp 
    Private/TEVKGraphicContext.cpp
    Private/TEVKLogicDevice.cpp
    Private/TEVKQueue.cpp
    Private/TEVKSwapChain.cpp
    Private/TEVKRenderPass.cpp
    Private/TEVKFrameBuffer.cpp
    Private/TEVKImageView.cpp
)

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message("----> find vulkan success: " ${Vulkan_INCLUDE_DIRS})
endif()


target_include_directories(TEWindow
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>
        $<INSTALL_INTERFACE:Public>
        ${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.4/include
        ${Vulkan_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(TEWindow 
    PUBLIC

    PRIVATE
        TELog
        glfw
        ${Vulkan_LIBRARY}
)


