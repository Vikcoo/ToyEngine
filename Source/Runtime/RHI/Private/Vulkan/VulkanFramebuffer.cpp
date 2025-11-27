// Vulkan Framebuffer 实现
#include "VulkanFramebuffer.h"
#include "VulkanDevice.h"
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
        throw std::runtime_error("Invalid attachment count");
    }
    
    vk::FramebufferCreateInfo createInfo;
    createInfo.setRenderPass(*renderPass.GetHandle());
    createInfo.setAttachments(attachments);
    createInfo.setWidth(extent.width);
    createInfo.setHeight(extent.height);
    createInfo.setLayers(1);
    
    try {
        m_framebuffer = m_device->GetHandle().createFramebuffer(createInfo);
        TE_LOG_DEBUG("Framebuffer created: {}x{} with {} attachment(s)", 
                    extent.width, extent.height, attachments.size());
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create framebuffer: {}", e.what());
        throw;
    }
}

VulkanFramebuffer::~VulkanFramebuffer() {
    TE_LOG_DEBUG("Framebuffer destroyed");
}

} // namespace TE

