// Vulkan Render Pass - 渲染通道
#pragma once

#include "../Core/VulkanDevice.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace TE {

class VulkanDevice;

/// 附件配置
struct AttachmentConfig {
    vk::Format format;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR;
};

/// Vulkan 渲染通道 - 管理渲染流程
class VulkanRenderPass {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    VulkanRenderPass(PrivateTag,
                     std::shared_ptr<VulkanDevice> device,
                     const std::vector<AttachmentConfig>& attachments);

    ~VulkanRenderPass();

    // 禁用拷贝，允许移动
    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
    VulkanRenderPass(VulkanRenderPass&&) noexcept = default;
    VulkanRenderPass& operator=(VulkanRenderPass&&) noexcept = default;

    // 获取句柄
    [[nodiscard]] const vk::raii::RenderPass& GetHandle() const { return m_renderPass; }

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::RenderPass m_renderPass{nullptr};
};

} // namespace TE

