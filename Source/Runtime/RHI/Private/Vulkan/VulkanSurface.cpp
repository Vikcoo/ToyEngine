// Vulkan Surface 实现
#define NOMINMAX
#include "VulkanSurface.h"
#include "VulkanContext.h"
#include "VulkanPhysicalDevice.h"
#include "Log/Log.h"
#include <algorithm>
#include <limits>

namespace TE {

// ============================================================================
// 构造/析构
// ============================================================================

VulkanSurface::VulkanSurface(PrivateTag, std::weak_ptr<VulkanContext> context,
                             vk::raii::SurfaceKHR surface)
    : m_context(std::move(context))
    , m_surface(std::move(surface))
{}

VulkanSurface::~VulkanSurface() {
    TE_LOG_DEBUG("Surface destroyed");
}

// ============================================================================
// 能力查询
// ============================================================================

SurfaceCapabilities VulkanSurface::QueryCapabilities(const VulkanPhysicalDevice& device) const {
    SurfaceCapabilities caps;
    caps.capabilities = device.GetHandle().getSurfaceCapabilitiesKHR(*m_surface);
    caps.formats = device.GetHandle().getSurfaceFormatsKHR(*m_surface);
    caps.presentModes = device.GetHandle().getSurfacePresentModesKHR(*m_surface);
    
    TE_LOG_DEBUG("Surface Capabilities:");
    TE_LOG_DEBUG("  Formats: {}", caps.formats.size());
    TE_LOG_DEBUG("  Present Modes: {}", caps.presentModes.size());
    TE_LOG_DEBUG("  Image Count: {} - {}", 
                 caps.capabilities.minImageCount,
                 caps.capabilities.maxImageCount);
    
    return caps;
}

vk::SurfaceCapabilitiesKHR VulkanSurface::GetCapabilities(const VulkanPhysicalDevice& device) const {
    return device.GetHandle().getSurfaceCapabilitiesKHR(*m_surface);
}

std::vector<vk::SurfaceFormatKHR> VulkanSurface::GetFormats(const VulkanPhysicalDevice& device) const {
    return device.GetHandle().getSurfaceFormatsKHR(*m_surface);
}

std::vector<vk::PresentModeKHR> VulkanSurface::GetPresentModes(const VulkanPhysicalDevice& device) const {
    return device.GetHandle().getSurfacePresentModesKHR(*m_surface);
}

// ============================================================================
// 最佳配置选择
// ============================================================================

vk::SurfaceFormatKHR VulkanSurface::ChooseBestFormat(
    const VulkanPhysicalDevice& device,
    const vk::Format preferredFormat,
    const vk::ColorSpaceKHR preferredColorSpace) const
{
    const auto formats = GetFormats(device);
    
    if (formats.empty()) {
        TE_LOG_ERROR("No surface formats available");
        return {vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear};
    }
    
    // 查找完全匹配
    for (const auto& format : formats) {
        if (format.format == preferredFormat && format.colorSpace == preferredColorSpace) {
            TE_LOG_DEBUG("Format: {} (preferred)", vk::to_string(format.format));
            return format;
        }
    }
    
    // 查找格式匹配
    for (const auto& format : formats) {
        if (format.format == preferredFormat) {
            TE_LOG_WARN("Format: {} (color space mismatch)", vk::to_string(format.format));
            return format;
        }
    }
    
    // 使用第一个可用格式
    TE_LOG_WARN("Format: {} (fallback)", vk::to_string(formats[0].format));
    return formats[0];
}

vk::PresentModeKHR VulkanSurface::ChooseBestPresentMode(
    const VulkanPhysicalDevice& device,
    const vk::PresentModeKHR preferredMode) const
{
    const auto modes = GetPresentModes(device);
    
    if (modes.empty()) {
        TE_LOG_ERROR("No present modes available");
        return vk::PresentModeKHR::eFifo;
    }
    
    // 查找首选模式
    for (const auto& mode : modes) {
        if (mode == preferredMode) {
            TE_LOG_DEBUG("Present Mode: {} (preferred)", vk::to_string(mode));
            return mode;
        }
    }
    
    // MAILBOX 备选方案：IMMEDIATE
    if (preferredMode == vk::PresentModeKHR::eMailbox) {
        for (const auto& mode : modes) {
            if (mode == vk::PresentModeKHR::eImmediate) {
                TE_LOG_WARN("Present Mode: IMMEDIATE (fallback)");
                return mode;
            }
        }
    }
    
    // 最后回退到 FIFO（必定支持）
    TE_LOG_WARN("Present Mode: FIFO (fallback)");
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSurface::ChooseExtent(
    const VulkanPhysicalDevice& device,
    const uint32_t desiredWidth,
    const uint32_t desiredHeight) const
{
    const auto capabilities = GetCapabilities(device);
    
    // 如果 currentExtent 不是特殊值，直接使用
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        TE_LOG_DEBUG("Extent: {}x{} (from surface)", 
                    capabilities.currentExtent.width,
                    capabilities.currentExtent.height);
        return capabilities.currentExtent;
    }
    
    // 在 min 和 max 之间选择
    vk::Extent2D extent = {desiredWidth, desiredHeight};
    
    extent.width = std::clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    
    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );
    
    TE_LOG_DEBUG("Extent: {}x{} (clamped from {}x{})",
                extent.width, extent.height,
                desiredWidth, desiredHeight);
    
    return extent;
}

} // namespace TE

