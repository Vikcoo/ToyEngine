//
// Created by yukai on 2025/8/17.
//
#include "Vulkan/TEVKRenderPass.h"

#include "Vulkan/TEVKFrameBuffer.h"
#include "Vulkan/TEVKLogicDevice.h"

namespace TE {
    TE::TEVKRenderPass::TEVKRenderPass(TEVKLogicDevice &logicDevice, const vk::AttachmentDescription &attachment, RenderSubPass renderSubPass)
    :m_renderSubPass(renderSubPass), m_attachment(attachment), m_logicDevice(logicDevice) {
        // default pass and attachment
        vk::AttachmentDescription defaultColorAttachment;
        defaultColorAttachment.setFormat(logicDevice.GetSetting().format);
        defaultColorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        defaultColorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        defaultColorAttachment.setStencilLoadOp( vk::AttachmentLoadOp::eDontCare );
        defaultColorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        defaultColorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
        defaultColorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
        defaultColorAttachment.setSamples(vk::SampleCountFlagBits::e1);
        m_attachment = defaultColorAttachment;

        vk::AttachmentReference colorAttachmentRef;
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.setColorAttachments( colorAttachmentRef );

        vk::SubpassDependency dependencies;
        dependencies.setSrcSubpass(vk::SubpassExternal);
        dependencies.setDstSubpass(0);
        dependencies.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependencies.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        dependencies.setSrcAccessMask({});
        dependencies.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        // create info
        vk::RenderPassCreateInfo renderPassInfo;

        renderPassInfo.setAttachments(m_attachment);
        renderPassInfo.setSubpasses(subpass);
        renderPassInfo.setDependencies(dependencies);

        m_handle = logicDevice.GetHandle()->createRenderPass(renderPassInfo);
        LOG_TRACE("renderPass : {}, {}", __FUNCTION__,reinterpret_cast<uint64_t>(static_cast<void*>(*m_handle)));
    }

    void TEVKRenderPass::Begin(vk::raii::CommandBuffer &commandBuffer, TEVKFrameBuffer &frameBuffer,const std::vector<vk::ClearValue> &clearValue) const {
        vk::Rect2D rect;
        rect.setOffset({0,0})
            .setExtent({frameBuffer.GetWidth(),frameBuffer.GetHeight()});

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.setRenderPass(*m_handle);
        renderPassInfo.setFramebuffer(*frameBuffer.GetHandle());
        renderPassInfo.setRenderArea(rect);
        renderPassInfo.setClearValues(clearValue);

        commandBuffer.beginRenderPass(renderPassInfo,vk::SubpassContents::eInline);
    }

    void TEVKRenderPass::End(vk::raii::CommandBuffer &commandBuffer) const {
        commandBuffer.endRenderPass();
    }
}
