// Vulkan Framebuffer 实现
#include "VulkanFramebuffer.h"
#include "../Core/VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "Log/Log.h"

namespace TE {

VulkanFramebuffer::VulkanFramebuffer(PrivateTag,
                                     std::shared_ptr<VulkanDevice> device,
                                     const VulkanRenderPass& renderPass,
                                     const std::vector<vk::ImageView>& attachments,
                                     const vk::Extent2D extent)
    : m_device(std::move(device))
    , m_extent(extent)
{
    if (attachments.empty()) {
        TE_LOG_ERROR("Framebuffer requires at least one attachment");
        return;
    }
    
    vk::FramebufferCreateInfo createInfo;
    createInfo.setRenderPass(*renderPass.GetHandle())
              .setAttachments(attachments)
              .setWidth(extent.width)
              .setHeight(extent.height)
              .setLayers(1);
    

    m_framebuffer = m_device->GetHandle().createFramebuffer(createInfo);
    if (m_framebuffer == nullptr){
        TE_LOG_ERROR("Failed to create framebuffer");
    }
    TE_LOG_DEBUG("Framebuffer created: {}x{} with {} attachment(s)",
                extent.width, extent.height, attachments.size());
}

VulkanFramebuffer::~VulkanFramebuffer() {
    TE_LOG_DEBUG("Framebuffer destroyed");
}

} // namespace TE

