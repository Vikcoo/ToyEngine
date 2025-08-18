//
// Created by yukai on 2025/8/17.
//
#include "TEVKRenderPass.h"

#include "TEVKLogicDevice.h"

namespace TE {
    TE::TEVKRenderPass::TEVKRenderPass(TEVKLogicDevice *logicDevice, const std::vector<vk::AttachmentDescription> &attachments, std::vector<RenderSubPass> renderSubPasses)
    :m_renderSubPasses(renderSubPasses), m_attachments(attachments), m_logicDevice(logicDevice) {
        // default pass and attachment
        if (m_renderSubPasses.empty()) {
            if (m_attachments.empty()) {
                vk::AttachmentDescription defaultColorAttachment;
                defaultColorAttachment.setFormat(logicDevice->GetSetting().format);
                defaultColorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
                defaultColorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
                defaultColorAttachment.setStencilLoadOp( vk::AttachmentLoadOp::eDontCare );
                defaultColorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
                defaultColorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
                defaultColorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
                defaultColorAttachment.setSamples(vk::SampleCountFlagBits::e1);
                m_attachments.push_back(defaultColorAttachment);
            }
            RenderSubPass renderSubPass;
            renderSubPass.colorAttachment = {0, vk::ImageLayout::eColorAttachmentOptimal};
            m_renderSubPasses.push_back(renderSubPass);
        }
        // subpass
        std::vector<vk::SubpassDescription> subpassDescriptions(m_renderSubPasses.size());
        for (int i = 0; i < m_renderSubPasses.size(); ++i) {
            auto subPass = m_renderSubPasses[i];
            if (subPass.inputAttachment.ref >= 0 && subPass.inputAttachment.layout == vk::ImageLayout::eUndefined) {
                subPass.inputAttachment.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
            }
            if (subPass.colorAttachment.ref >= 0 && subPass.colorAttachment.layout == vk::ImageLayout::eUndefined) {
                subPass.colorAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
            }
            if (subPass.depthStencilAttachment.ref >= 0 && subPass.depthStencilAttachment.layout == vk::ImageLayout::eUndefined) {
                subPass.depthStencilAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            }
            if (subPass.resolveAttachment.ref >= 0 && subPass.resolveAttachment.layout == vk::ImageLayout::eUndefined) {
                subPass.resolveAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
            }

            std::vector<vk::AttachmentReference> inputAttachmentRef = {{static_cast<uint32_t>(subPass.inputAttachment.ref),subPass.inputAttachment.layout}};
            std::vector<vk::AttachmentReference> colorAttachmentRef = {{static_cast<uint32_t>(subPass.colorAttachment.ref),subPass.colorAttachment.layout}};
            std::vector<vk::AttachmentReference> depthStencilAttachmentRef = {{static_cast<uint32_t>(subPass.depthStencilAttachment.ref),subPass.depthStencilAttachment.layout}};
            std::vector<vk::AttachmentReference> resolveAttachmentRef = {{static_cast<uint32_t>(subPass.resolveAttachment.ref),subPass.resolveAttachment.layout}};

            auto a =std::vector<vk::AttachmentReference>();
            //subpassDescriptions[i].flags = 0;
            subpassDescriptions[i].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            //subpassDescriptions[i].setInputAttachmentCount(subPass.inputAttachment.ref >= 0 ? inputAttachmentRef.size() : 0);
            subpassDescriptions[i].setPInputAttachments(subPass.inputAttachment.ref >= 0 ? inputAttachmentRef.data() : nullptr);
            //subpassDescriptions[i].setColorAttachmentCount(subPass.colorAttachment.ref >= 0 ? colorAttachmentRef.size() : 0);
            subpassDescriptions[i].setPColorAttachments(subPass.colorAttachment.ref >= 0 ? colorAttachmentRef.data() : nullptr);
            subpassDescriptions[i].setResolveAttachments(subPass.resolveAttachment.ref >= 0 ? resolveAttachmentRef : a);
            subpassDescriptions[i].setPDepthStencilAttachment(subPass.depthStencilAttachment.ref >= 0 ? depthStencilAttachmentRef.data() : nullptr);
            subpassDescriptions[i].setPreserveAttachmentCount(0);
            subpassDescriptions[i].setPPreserveAttachments(nullptr);
        }

        // create info
        vk::RenderPassCreateInfo renderPassInfo;
        std::vector<vk::SubpassDependency> dependencies(m_renderSubPasses.size() - 1);

        if (m_renderSubPasses.size() > 1) {
            for (int i = 0; i < dependencies.size(); ++i) {
                dependencies[i].setSrcSubpass(i);
                dependencies[i].setDstSubpass(i+1);
                dependencies[i].setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
                dependencies[i].setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader);
                dependencies[i].setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
                dependencies[i].setDstAccessMask(vk::AccessFlagBits::eInputAttachmentRead);
                dependencies[i].setDependencyFlags(vk::DependencyFlagBits::eByRegion);
            }
        }


        //renderPassInfo.attachmentCount = m_attachments.size();
        renderPassInfo.setAttachments(m_attachments);
        //renderPassInfo.setSubpassCount(m_renderSubPasses.size());
        renderPassInfo.setSubpasses(subpassDescriptions);
        renderPassInfo.setDependencyCount(dependencies.size());
        renderPassInfo.setDependencies(dependencies);

        m_handle = logicDevice->GetHandle()->createRenderPass(renderPassInfo);
        LOG_TRACE("renderPass : {}, {}, attachments.size :{}, SubPasses.size :{}", __FUNCTION__,(void*) m_handle, m_attachments.size(), m_renderSubPasses.size());
    }

    TEVKRenderPass::~TEVKRenderPass() {
        m_logicDevice->GetHandle()->destroyRenderPass(m_handle);
    }
}
