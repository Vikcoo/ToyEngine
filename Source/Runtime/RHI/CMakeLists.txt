project(RHI)

# 查找 Vulkan SDK
find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found! Download from: https://vulkan.lunarg.com/")
endif()

# 创建 RHI 静态库
add_library(RHI STATIC
    # Vulkan 实现 - 层次1：初始化
    Private/Vulkan/Core/VulkanUtils.h
    Private/Vulkan/Core/VulkanUtils.cpp
    Private/Vulkan/Core/VulkanContext.h
    Private/Vulkan/Core/VulkanContext.cpp
    Private/Vulkan/Core/VulkanPhysicalDevice.h
    Private/Vulkan/Core/VulkanPhysicalDevice.cpp
    Private/Vulkan/Core/VulkanSurface.h
    Private/Vulkan/Core/VulkanSurface.cpp
    
    # 层次2：设备和队列
    Private/Vulkan/Core/VulkanDevice.h
    Private/Vulkan/Core/VulkanDevice.cpp
    Private/Vulkan/Core/VulkanQueue.h
    Private/Vulkan/Core/VulkanQueue.cpp
    Private/Vulkan/Commands/VulkanCommandPool.h
    Private/Vulkan/Commands/VulkanCommandPool.cpp
    
    # 层次3：资源对象
    Private/Vulkan/Resources/VulkanSwapChain.h
    Private/Vulkan/Resources/VulkanSwapChain.cpp
    Private/Vulkan/Resources/VulkanImageView.h
    Private/Vulkan/Resources/VulkanImageView.cpp
    Private/Vulkan/Pipeline/VulkanRenderPass.h
    Private/Vulkan/Pipeline/VulkanRenderPass.cpp
    Private/Vulkan/Pipeline/VulkanFramebuffer.h
    Private/Vulkan/Pipeline/VulkanFramebuffer.cpp
    Private/Vulkan/Resources/VulkanBuffer.h
    Private/Vulkan/Resources/VulkanBuffer.cpp
    Private/Vulkan/Descriptors/VulkanDescriptorSetLayout.h
    Private/Vulkan/Descriptors/VulkanDescriptorSetLayout.cpp
    Private/Vulkan/Descriptors/VulkanDescriptorPool.h
    Private/Vulkan/Descriptors/VulkanDescriptorPool.cpp
    
    # 层次4：命令和管线
    Private/Vulkan/Pipeline/VulkanShader.h
    Private/Vulkan/Pipeline/VulkanShader.cpp
    Private/Vulkan/Pipeline/VulkanPipeline.h
    Private/Vulkan/Pipeline/VulkanPipeline.cpp
    Private/Vulkan/Commands/VulkanCommandBuffer.h
    Private/Vulkan/Commands/VulkanCommandBuffer.cpp
    Private/Vulkan/Resources/VulkanTexture2D.cpp
    Private/Vulkan/Resources/VulkanTexture2D.h
    Private/Vulkan/Resources/VulkanImage.cpp
    Private/Vulkan/Resources/VulkanImage.h
    Public/Resources/RHITexture.h
)

# 包含目录
target_include_directories(RHI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan  # 暴露 Vulkan 头文件
    ${Vulkan_INCLUDE_DIRS}  # 暴露 Vulkan SDK（VulkanContext.h 依赖 vulkan_raii.hpp）
)

target_include_directories(RHI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 链接库
target_link_libraries(RHI 
    PUBLIC
        Core
        Platform
    PRIVATE
        Vulkan::Vulkan
        glfw
        Asset
)

# 编译定义
if(WIN32)
    target_compile_definitions(RHI PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

message(STATUS "RHI module configured")
message(STATUS "  Vulkan: ${Vulkan_INCLUDE_DIRS}")
