// Vulkan Surface - 窗口表面管理
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace TE {

class VulkanContext;
class VulkanPhysicalDevice;

/// Surface 能力信息
struct SurfaceCapabilities {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

/// Vulkan Surface - 管理窗口表面
class VulkanSurface {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanContext;
    };

    explicit VulkanSurface(PrivateTag, std::weak_ptr<VulkanContext> context,
                          vk::raii::SurfaceKHR surface);

    ~VulkanSurface();

    // 禁用拷贝，允许移动
    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;
    VulkanSurface(VulkanSurface&&) noexcept = default;
    VulkanSurface& operator=(VulkanSurface&&) noexcept = default;

    // 查询能力
    [[nodiscard]] SurfaceCapabilities QueryCapabilities(const VulkanPhysicalDevice& device) const;
    [[nodiscard]] vk::SurfaceCapabilitiesKHR GetCapabilities(const VulkanPhysicalDevice& device) const;
    [[nodiscard]] std::vector<vk::SurfaceFormatKHR> GetFormats(const VulkanPhysicalDevice& device) const;
    [[nodiscard]] std::vector<vk::PresentModeKHR> GetPresentModes(const VulkanPhysicalDevice& device) const;

    // 选择最佳配置
    [[nodiscard]] vk::SurfaceFormatKHR ChooseBestFormat(
        const VulkanPhysicalDevice& device,
        vk::Format preferredFormat = vk::Format::eB8G8R8A8Srgb,
        vk::ColorSpaceKHR preferredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear
    ) const;
    
    [[nodiscard]] vk::PresentModeKHR ChooseBestPresentMode(
        const VulkanPhysicalDevice& device,
        vk::PresentModeKHR preferredMode = vk::PresentModeKHR::eMailbox
    ) const;
    
    [[nodiscard]] vk::Extent2D ChooseExtent(
        const VulkanPhysicalDevice& device,
        uint32_t desiredWidth,
        uint32_t desiredHeight
    ) const;

    // 获取底层句柄
    [[nodiscard]] const vk::raii::SurfaceKHR& GetHandle() const { return m_surface; }

private:
    std::weak_ptr<VulkanContext> m_context;
    vk::raii::SurfaceKHR m_surface{nullptr};
};

} // namespace TE

