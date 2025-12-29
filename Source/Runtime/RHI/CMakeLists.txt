project(RHI)

# 查找 Vulkan SDK
find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found! Download from: https://vulkan.lunarg.com/")
endif()

# 创建 RHI 静态库
add_library(RHI STATIC
    # Vulkan 实现 - 层次1：初始化
    Private/Vulkan/VulkanUtils.h
    Private/Vulkan/VulkanUtils.cpp
    Private/Vulkan/VulkanContext.h
    Private/Vulkan/VulkanContext.cpp
    Private/Vulkan/VulkanPhysicalDevice.h
    Private/Vulkan/VulkanPhysicalDevice.cpp
    Private/Vulkan/VulkanSurface.h
    Private/Vulkan/VulkanSurface.cpp
    
    # 层次2：设备和队列
    Private/Vulkan/VulkanDevice.h
    Private/Vulkan/VulkanDevice.cpp
    Private/Vulkan/VulkanQueue.h
    Private/Vulkan/VulkanQueue.cpp
    Private/Vulkan/VulkanCommandPool.h
    Private/Vulkan/VulkanCommandPool.cpp
    
    # 层次3：资源对象
    Private/Vulkan/VulkanSwapChain.h
    Private/Vulkan/VulkanSwapChain.cpp
    Private/Vulkan/VulkanImageView.h
    Private/Vulkan/VulkanImageView.cpp
    Private/Vulkan/VulkanRenderPass.h
    Private/Vulkan/VulkanRenderPass.cpp
    Private/Vulkan/VulkanFramebuffer.h
    Private/Vulkan/VulkanFramebuffer.cpp
    Private/Vulkan/VulkanBuffer.h
    Private/Vulkan/VulkanBuffer.cpp
    Private/Vulkan/VulkanDescriptorSetLayout.h
    Private/Vulkan/VulkanDescriptorSetLayout.cpp
    Private/Vulkan/VulkanDescriptorPool.h
    Private/Vulkan/VulkanDescriptorPool.cpp
    
    # 层次4：命令和管线
    Private/Vulkan/VulkanShader.h
    Private/Vulkan/VulkanShader.cpp
    Private/Vulkan/VulkanPipeline.h
    Private/Vulkan/VulkanPipeline.cpp
    Private/Vulkan/VulkanCommandBuffer.h
    Private/Vulkan/VulkanCommandBuffer.cpp
        Private/Vulkan/VulkanTexture2D.cpp
        Private/Vulkan/VulkanTexture2D.h
        Private/Vulkan/VulkanImage.cpp
        Private/Vulkan/VulkanImage.h
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
)

# 编译定义
if(WIN32)
    target_compile_definitions(RHI PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

message(STATUS "RHI module configured")
message(STATUS "  Vulkan: ${Vulkan_INCLUDE_DIRS}")
