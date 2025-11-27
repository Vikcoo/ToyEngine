// Vulkan Framebuffer - 帧缓冲
#pragma once

#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace TE {

class VulkanDevice;
class VulkanRenderPass;
class VulkanImageView;

/// Vulkan 帧缓冲 - 绑定 RenderPass 和 ImageView
class VulkanFramebuffer {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    explicit VulkanFramebuffer(PrivateTag,
                              std::shared_ptr<VulkanDevice> device,
                              const VulkanRenderPass& renderPass,
                              const std::vector<vk::ImageView>& attachments,
                              vk::Extent2D extent);

    ~VulkanFramebuffer();

    // 禁用拷贝，允许移动
    VulkanFramebuffer(const VulkanFramebuffer&) = delete;
    VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;
    VulkanFramebuffer(VulkanFramebuffer&&) noexcept = default;
    VulkanFramebuffer& operator=(VulkanFramebuffer&&) noexcept = default;

    // 获取信息
    [[nodiscard]] const vk::raii::Framebuffer& GetHandle() const { return m_framebuffer; }
    [[nodiscard]] vk::Extent2D GetExtent() const { return m_extent; }

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::Framebuffer m_framebuffer{nullptr};
    vk::Extent2D m_extent;
};

} // namespace TE

