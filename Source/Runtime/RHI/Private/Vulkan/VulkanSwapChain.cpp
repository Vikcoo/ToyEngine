// Vulkan Swap Chain 实现
#include "VulkanSwapChain.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanImageView.h"
#include "VulkanQueue.h"
#include "Log/Log.h"
#include <algorithm>

namespace TE {

VulkanSwapChain::VulkanSwapChain(PrivateTag,
                                 std::shared_ptr<VulkanDevice> device,
                                 VulkanSurface& surface,
                                 const SwapChainConfig& config,
                                 const uint32_t desiredWidth,
                                 const uint32_t desiredHeight)
    : m_device(std::move(device))
    , m_surface(&surface)
{
    Initialize(config, desiredWidth, desiredHeight);
}

VulkanSwapChain::~VulkanSwapChain() {
    TE_LOG_DEBUG("Swap chain destroyed");
}

void VulkanSwapChain::Initialize(const SwapChainConfig& config, 
                                 const uint32_t desiredWidth, 
                                 const uint32_t desiredHeight) {
    const auto& physicalDevice = m_device->GetPhysicalDevice();
    
    // 查询 Surface 能力
    const auto capabilities = m_surface->GetCapabilities(physicalDevice);
    const auto formats = m_surface->GetFormats(physicalDevice);
    const auto presentModes = m_surface->GetPresentModes(physicalDevice);
    
    if (formats.empty() || presentModes.empty()) {
        TE_LOG_ERROR("Surface has no formats or present modes");
        throw std::runtime_error("Invalid surface capabilities");
    }
    
    // 选择格式
    const auto surfaceFormat = m_surface->ChooseBestFormat(
        physicalDevice,
        config.preferredFormat,
        config.preferredColorSpace
    );
    m_format = surfaceFormat.format;
    m_colorSpace = surfaceFormat.colorSpace;
    
    // 选择呈现模式
    const auto presentMode = m_surface->ChooseBestPresentMode(
        physicalDevice,
        config.preferredPresentMode
    );
    
    // 选择图像数量
    uint32_t imageCount = config.imageCount;
    if (imageCount < capabilities.minImageCount) {
        imageCount = capabilities.minImageCount;
    }
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    
    // 选择范围
    m_extent = m_surface->ChooseExtent(physicalDevice, desiredWidth, desiredHeight);
    
    // 检查共享模式
    const auto& queueFamilies = m_device->GetQueueFamilies();
    const bool isSameQueueFamily = queueFamilies.IsSameGraphicsPresent();
    
    vk::SharingMode sharingMode;
    std::vector<uint32_t> queueFamilyIndices;
    
    if (isSameQueueFamily) {
        sharingMode = vk::SharingMode::eExclusive;
    } else {
        sharingMode = vk::SharingMode::eConcurrent;
        if (queueFamilies.graphics.has_value()) {
            queueFamilyIndices.push_back(queueFamilies.graphics.value());
        }
        if (queueFamilies.present.has_value()) {
            queueFamilyIndices.push_back(queueFamilies.present.value());
        }
    }
    
    // 创建交换链
    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.setSurface(*m_surface->GetHandle());
    createInfo.setMinImageCount(imageCount);
    createInfo.setImageFormat(m_format);
    createInfo.setImageColorSpace(m_colorSpace);
    createInfo.setImageExtent(m_extent);
    createInfo.setImageArrayLayers(1);
    createInfo.setImageUsage(config.usage);
    createInfo.setImageSharingMode(sharingMode);
    createInfo.setQueueFamilyIndices(queueFamilyIndices);
    createInfo.setPreTransform(capabilities.currentTransform);
    createInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    createInfo.setPresentMode(presentMode);
    createInfo.setClipped(VK_TRUE);
    
    try {
        m_swapchain = m_device->GetHandle().createSwapchainKHR(createInfo);
        m_images = m_swapchain.getImages();
        
        TE_LOG_INFO("Swap chain created:");
        TE_LOG_INFO("  Format: {}", vk::to_string(m_format));
        TE_LOG_INFO("  Extent: {}x{}", m_extent.width, m_extent.height);
        TE_LOG_INFO("  Image Count: {}", m_images.size());
        TE_LOG_INFO("  Present Mode: {}", vk::to_string(presentMode));
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create swap chain: {}", e.what());
        throw;
    }
}

std::unique_ptr<VulkanSwapChain> VulkanSwapChain::Recreate(const SwapChainConfig& config) const {
    return std::make_unique<VulkanSwapChain>(
        PrivateTag{},
        m_device,
        *m_surface,
        config,
        m_extent.width,
        m_extent.height
    );
}

SwapChainAcquireResult VulkanSwapChain::AcquireNextImage(
    const vk::Semaphore signalSemaphore,
    const vk::Fence fence,
    const uint64_t timeout) const
{
    try {
        const auto [result, imageIndex] = m_swapchain.acquireNextImage(timeout, signalSemaphore, fence);
        return {imageIndex, result};
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to acquire next image: {}", e.what());
        return {0, vk::Result::eErrorUnknown};
    }
}

vk::Result VulkanSwapChain::Present(
    const uint32_t imageIndex,
    VulkanQueue& presentQueue,
    const std::vector<vk::Semaphore>& waitSemaphores) const
{
    vk::PresentInfoKHR presentInfo;
    presentInfo.setSwapchains(*m_swapchain);
    presentInfo.setImageIndices(imageIndex);
    presentInfo.setWaitSemaphores(waitSemaphores);
    
    try {
        return presentQueue.GetHandle().presentKHR(presentInfo);
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to present: {}", e.what());
        return vk::Result::eErrorUnknown;
    }
}

std::unique_ptr<VulkanImageView> VulkanSwapChain::CreateImageView(const uint32_t imageIndex) const {
    if (imageIndex >= m_images.size()) {
        TE_LOG_ERROR("Invalid image index: {}", imageIndex);
        return nullptr;
    }
    
    return std::make_unique<VulkanImageView>(
        VulkanImageView::PrivateTag{},
        m_device,
        m_images[imageIndex],
        m_format,
        vk::ImageAspectFlagBits::eColor
    );
}

} // namespace TE

