//
// Created by yukai on 2025/8/17.
//
#pragma once
#include "TEVKCommon.h"


namespace TE {
    class TEVKLogicDevice;

    struct Attachment {
        int ref = -1;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    };
    struct RenderSubPass {
        Attachment inputAttachment;
        Attachment colorAttachment;
        Attachment depthStencilAttachment;
        Attachment resolveAttachment;
    };
    class TEVKRenderPass {
        public:
        TEVKRenderPass(TEVKLogicDevice* logicDevice, const std::vector<vk::AttachmentDescription>& attachments = {}, std::vector<RenderSubPass> renderSubPasses = {});
        ~TEVKRenderPass();

        vk::RenderPass GetHandle() const { return m_handle;}
    private:
        vk::RenderPass m_handle = VK_NULL_HANDLE;
        TEVKLogicDevice* m_logicDevice;

        std::vector<RenderSubPass> m_renderSubPasses;
        std::vector<vk::AttachmentDescription> m_attachments;
    };
}
