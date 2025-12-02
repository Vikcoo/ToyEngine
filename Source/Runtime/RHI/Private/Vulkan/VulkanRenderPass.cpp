// Vulkan Render Pass 实现
#include "VulkanRenderPass.h"
#include "VulkanDevice.h"
#include "Log/Log.h"

namespace TE {

VulkanRenderPass::VulkanRenderPass(PrivateTag,
                                   std::shared_ptr<VulkanDevice> device,
                                   const std::vector<AttachmentConfig>& attachments)
    : m_device(std::move(device))
{
    if (attachments.empty()) {
        TE_LOG_ERROR("Render pass requires at least one attachment");
        throw std::runtime_error("Invalid attachment count");
    }
    
    // 创建附件描述
    std::vector<vk::AttachmentDescription> attachmentDescriptions;
    attachmentDescriptions.reserve(attachments.size());
    
    for (const auto& config : attachments) {
        vk::AttachmentDescription desc;
        desc.setFormat(config.format);
        desc.setSamples(config.samples);
        desc.setLoadOp(config.loadOp);
        desc.setStoreOp(config.storeOp);
        desc.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        desc.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        desc.setInitialLayout(config.initialLayout);
        desc.setFinalLayout(config.finalLayout);
        attachmentDescriptions.push_back(desc);
    }
    
    // 创建子通道引用
    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.setAttachment(0);
    colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    
    // 创建子通道
    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(colorAttachmentRef);
    
    // 创建子通道依赖
    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setDstSubpass(0);
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    
    // 创建 RenderPass
    vk::RenderPassCreateInfo createInfo;
    createInfo.setAttachments(attachmentDescriptions)
              .setSubpasses(subpass)
              .setDependencies(dependency);
    
    try {
        m_renderPass = m_device->GetHandle().createRenderPass(createInfo);
        TE_LOG_DEBUG("Render pass created with {} attachment(s)", attachments.size());
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create render pass: {}", e.what());
        throw;
    }
}

VulkanRenderPass::~VulkanRenderPass() {
    TE_LOG_DEBUG("Render pass destroyed");
}

} // namespace TE

