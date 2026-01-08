// Vulkan Image View - 图像视图
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace TE {

class VulkanDevice;
class VulkanImage;

struct VulkanImageViewConfig{
    vk::Image image;
    vk::Format format;
    uint32_t mipLevelCount;
    vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
};

/// Vulkan 图像视图 - 为 Image 创建视图
class VulkanImageView {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
        friend class VulkanSwapChain;
        friend class VulkanTexture2D;
        friend class VulkanImage;
    };

    VulkanImageView(PrivateTag, std::shared_ptr<VulkanDevice> device,const VulkanImageViewConfig& config);

    ~VulkanImageView();

    // 禁用拷贝，允许移动
    VulkanImageView(const VulkanImageView&) = delete;
    VulkanImageView& operator=(const VulkanImageView&) = delete;
    VulkanImageView(VulkanImageView&&) noexcept = default;
    VulkanImageView& operator=(VulkanImageView&&) noexcept = default;

    // 获取句柄
    [[nodiscard]] const vk::raii::ImageView& GetHandle() const { return m_imageView; }

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::ImageView m_imageView{nullptr};
};

} // namespace TE

