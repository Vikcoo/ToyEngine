// Vulkan Swap Chain - 交换链
#pragma once

#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace TE {

class VulkanDevice;
class VulkanSurface;
class VulkanImageView;

/// 交换链配置
struct SwapChainConfig {
    vk::Format preferredFormat = vk::Format::eB8G8R8A8Srgb;
    vk::ColorSpaceKHR preferredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eMailbox;
    uint32_t imageCount = 3;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment;
};

/// 交换链获取图像结果
struct SwapChainAcquireResult {
    uint32_t imageIndex;
    vk::Result result;
};

/// Vulkan 交换链 - 管理呈现图像
class VulkanSwapChain {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
        friend class VulkanSwapChain;
    };

    explicit VulkanSwapChain(PrivateTag,
                            std::shared_ptr<VulkanDevice> device,
                            VulkanSurface& surface,
                            const SwapChainConfig& config,
                            uint32_t desiredWidth = 1280,
                            uint32_t desiredHeight = 720);

    ~VulkanSwapChain();

    // 禁用拷贝，允许移动
    VulkanSwapChain(const VulkanSwapChain&) = delete;
    VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;
    VulkanSwapChain(VulkanSwapChain&&) noexcept = default;
    VulkanSwapChain& operator=(VulkanSwapChain&&) noexcept = default;

    // 重建交换链（返回新对象）
    [[nodiscard]] std::unique_ptr<VulkanSwapChain> Recreate(const SwapChainConfig& config) const;

    // 获取图像
    [[nodiscard]] SwapChainAcquireResult AcquireNextImage(
        vk::Semaphore signalSemaphore,
        vk::Fence fence = nullptr,
        uint64_t timeout = UINT64_MAX
    ) const;

    // 呈现图像
    [[nodiscard]] vk::Result Present(
        uint32_t imageIndex,
        VulkanQueue& presentQueue,
        const std::vector<vk::Semaphore>& waitSemaphores = {}
    ) const;

    // 创建 ImageView（为交换链图像创建视图）
    [[nodiscard]] std::unique_ptr<VulkanImageView> CreateImageView(uint32_t imageIndex) const;

    // 获取信息
    [[nodiscard]] vk::Format GetFormat() const { return m_format; }
    [[nodiscard]] vk::Extent2D GetExtent() const { return m_extent; }
    [[nodiscard]] uint32_t GetImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    [[nodiscard]] vk::Image GetImage(uint32_t index) const { return m_images[index]; }
    [[nodiscard]] const std::vector<vk::Image>& GetImages() const { return m_images; }
    [[nodiscard]] const vk::raii::SwapchainKHR& GetHandle() const { return m_swapchain; }

private:
    void Initialize(const SwapChainConfig& config, uint32_t desiredWidth, uint32_t desiredHeight);

private:
    std::shared_ptr<VulkanDevice> m_device;
    VulkanSurface* m_surface;  // 不拥有，由外部管理
    
    vk::raii::SwapchainKHR m_swapchain{nullptr};
    std::vector<vk::Image> m_images;  // 不拥有，由 swapchain 管理
    vk::Format m_format;
    vk::ColorSpaceKHR m_colorSpace;
    vk::Extent2D m_extent;
};

} // namespace TE

