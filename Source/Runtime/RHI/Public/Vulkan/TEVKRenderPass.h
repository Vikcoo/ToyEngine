//
// Created by yukai on 2025/8/17.
//
#pragma once
#include "TEVKCommon.h"


namespace TE {
    class TEVKLogicDevice;
    class TEVKFrameBuffer;

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
        explicit TEVKRenderPass(TEVKLogicDevice& logicDevice, const vk::AttachmentDescription& attachment = {}, RenderSubPass renderSubPass = {});

        const vk::raii::RenderPass& GetHandle() const { return m_handle;}

        void Begin (vk::raii::CommandBuffer& commandBuffer, TEVKFrameBuffer& frameBuffer, const std::vector<vk::ClearValue>& clearValue) const;
        void End (vk::raii::CommandBuffer& commandBuffer) const;
    private:
        vk::raii::RenderPass m_handle{VK_NULL_HANDLE};
        TEVKLogicDevice& m_logicDevice;
        RenderSubPass m_renderSubPass;
        vk::AttachmentDescription m_attachment;
    };
}
