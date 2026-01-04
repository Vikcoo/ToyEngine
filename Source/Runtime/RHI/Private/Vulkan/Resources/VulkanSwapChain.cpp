// Vulkan Swap Chain 实现
#include "VulkanSwapChain.h"
#include "Core/VulkanDevice.h"
#include "Core/VulkanSurface.h"
#include "VulkanImageView.h"
#include "Core/VulkanQueue.h"
#include "Log/Log.h"
#include <algorithm>

namespace TE {

VulkanSwapChain::VulkanSwapChain(PrivateTag,
                                 std::shared_ptr<VulkanDevice> device,
                                 std::shared_ptr<VulkanSurface> surface,
                                 const SwapChainConfig& config,
                                 const uint32_t desiredWidth,
                                 const uint32_t desiredHeight,
                                 const vk::SwapchainKHR oldSwapChain)
    : m_device(std::move(device))
    , m_surface(std::move(surface))
{
    Initialize(config, desiredWidth, desiredHeight, oldSwapChain);
}

VulkanSwapChain::~VulkanSwapChain() {
    TE_LOG_DEBUG("Swap chain destroyed");
}



void VulkanSwapChain::Initialize(const SwapChainConfig& config,
                                 const uint32_t desiredWidth, 
                                 const uint32_t desiredHeight,
                                 vk::SwapchainKHR oldSwapChain) {
    if (!m_surface) {
        TE_LOG_ERROR("Surface is null during initialization");
        return;
    }
    
    const auto& physicalDevice = m_device->GetPhysicalDevice();
    
    // 查询 Surface 能力
    const auto capabilities = m_surface->GetCapabilities(physicalDevice);
    const auto formats = m_surface->GetFormats(physicalDevice);
    const auto presentModes = m_surface->GetPresentModes(physicalDevice);
    
    if (formats.empty() || presentModes.empty()) {
        TE_LOG_ERROR("Surface has no formats or present modes");
        return;
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
    createInfo.setSurface(*m_surface->GetHandle())
              .setMinImageCount(imageCount)
              .setImageFormat(m_format)
              .setImageColorSpace(m_colorSpace)
              .setImageExtent(m_extent)
              .setImageArrayLayers(1)
              .setImageUsage(config.usage)
              .setImageSharingMode(sharingMode)
              .setQueueFamilyIndices(queueFamilyIndices)
              .setPreTransform(capabilities.currentTransform)
              .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
              .setPresentMode(presentMode)
              .setClipped(VK_TRUE);
    
    // 如果提供了旧交换链，设置 oldSwapChain，这允许 Vulkan 驱动优雅地处理交换链过渡，无需显式销毁旧交换链
    if (oldSwapChain) {
        createInfo.setOldSwapchain(oldSwapChain);
    }

    m_swapChain = m_device->GetHandle().createSwapchainKHR(createInfo);
    if (m_swapChain == VK_NULL_HANDLE){
        TE_LOG_ERROR("Failed to create swap chain");
    }

    m_images = m_swapChain.getImages();
    TE_LOG_INFO("Swap chain created:");
    TE_LOG_INFO("  Format: {}", vk::to_string(m_format));
    TE_LOG_INFO("  Extent: {}x{}", m_extent.width, m_extent.height);
    TE_LOG_INFO("  Image Count: {}", m_images.size());
    TE_LOG_INFO("  Present Mode: {}", vk::to_string(presentMode));

    m_imageViews = CreateImageViews();
}

std::unique_ptr<VulkanSwapChain> VulkanSwapChain::Recreate(
    const SwapChainConfig& config,
    uint32_t desiredWidth,
    uint32_t desiredHeight) const {
    if (!m_surface) {
        TE_LOG_ERROR("Cannot recreate swap chain: surface is null");
        return nullptr;
    }
    
    // 获取旧交换链的句柄（在创建新交换链之前保存）
    vk::SwapchainKHR oldSwapChainHandle = *m_swapChain;
    
    // 使用新的窗口尺寸重建交换链，传递旧交换链句柄 Vulkan 会在新交换链创建成功后自动处理旧交换链的销毁
    return std::make_unique<VulkanSwapChain>(
        PrivateTag{},
        m_device,
        m_surface,  // 共享同一个surface
        config,
        desiredWidth,   // 使用新的窗口宽度
        desiredHeight,  // 使用新的窗口高度
        oldSwapChainHandle  // 传递旧交换链句柄，避免 ErrorNativeWindowInUseKHR
    );
}

SwapChainAcquireResult VulkanSwapChain::AcquireNextImage(
    const vk::Semaphore signalSemaphore,
    const vk::Fence fence,
    const uint64_t timeout) const
{
    const auto [result, imageIndex] = m_swapChain.acquireNextImage(timeout, signalSemaphore, fence);
    // 处理设备丢失和交换链过时
    if (result == vk::Result::eErrorDeviceLost) {
        TE_LOG_ERROR("Device lost during acquire next image");
        return {0, vk::Result::eErrorDeviceLost};
    }
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        TE_LOG_WARN("Swap chain out of date or suboptimal: {}", vk::to_string(result));
        return {imageIndex, result};
    }

    return {imageIndex, result};
}

vk::Result VulkanSwapChain::Present(
    const uint32_t imageIndex,
    const VulkanQueue& presentQueue,
    const std::vector<vk::Semaphore>& waitSemaphores) const
{
    vk::PresentInfoKHR presentInfo;
    presentInfo.setSwapchains(*m_swapChain)
               .setImageIndices(imageIndex)
               .setWaitSemaphores(waitSemaphores);

    const vk::Result result = presentQueue.GetHandle().presentKHR(presentInfo);

    // 处理设备丢失和交换链过时
    if (result == vk::Result::eErrorDeviceLost) {
        TE_LOG_ERROR("Device lost during present");
        return vk::Result::eErrorDeviceLost;
    }
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        TE_LOG_WARN("Swap chain out of date or suboptimal during present: {}", vk::to_string(result));
        return result;
    }

    return result;
}

std::vector<std::unique_ptr<VulkanImageView>> VulkanSwapChain::CreateImageViews() const {
    
    if (!m_device) {
        TE_LOG_ERROR("Device is null");
        return {};
    }

    const auto actualImageCount = GetImageCount();
    std::vector<std::unique_ptr<VulkanImageView>> imageViews;
    imageViews.reserve(actualImageCount);

    for (uint32_t i = 0; i < actualImageCount; ++i) {
        auto imageView = std::make_unique<VulkanImageView>(
                VulkanImageView::PrivateTag{},
                m_device,
                GetImage(i),
                m_format,
                vk::ImageAspectFlagBits::eColor
            );

        if (!imageView) {
            TE_LOG_ERROR("Failed to create image view {}", i);
            continue;
        }

        imageViews.emplace_back(std::move(imageView));
    }
    TE_LOG_INFO(" Created {} image view(s) (actual swap chain image count: {})",
                    actualImageCount, actualImageCount);
    return imageViews;
}

} // namespace TE

