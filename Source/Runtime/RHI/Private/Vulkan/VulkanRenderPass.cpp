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
        return;
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
    // 这个依赖确保：
    // 1. 在开始渲染通道前，图像布局从初始布局转换到颜色附件最优布局
    // 2. 在结束渲染通道后，图像布局从颜色附件最优布局转换到最终布局（通常是 ePresentSrcKHR）
    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);  // 外部子通道（Present 操作）
    dependency.setDstSubpass(0);                    // 我们的子通道
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);  // Present 阶段
    dependency.setSrcAccessMask(vk::AccessFlags{});  // Present 不需要访问标志
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);  // 颜色附件输出阶段
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);  // 允许写入颜色附件
    
    // 创建 RenderPass
    vk::RenderPassCreateInfo createInfo;
    createInfo.setAttachments(attachmentDescriptions)
              .setSubpasses(subpass)
              .setDependencies(dependency);
    

    m_renderPass = m_device->GetHandle().createRenderPass(createInfo);
    if (m_renderPass == nullptr){
        TE_LOG_ERROR("Failed to create render pass");
    }
    TE_LOG_DEBUG("Render pass created with {} attachment(s)", attachments.size());
}

VulkanRenderPass::~VulkanRenderPass() {
    TE_LOG_DEBUG("Render pass destroyed");
}

} // namespace TE

