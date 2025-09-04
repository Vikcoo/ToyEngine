/*
文件用途: 渲染通道封装
- 负责: 通过 Attachment/Subpass/Dependency 组装 RenderPass，驱动绘制流程
- 概念: RenderPass 约束子通道如何读写附件与布局转换；与 Framebuffer 搭配使用
*/
#pragma once
#include "TEVKCommon.h"

namespace TE {
    class TEVKLogicDevice;
    class TEVKFrameBuffer;

    // 附件在子通道中的引用
    struct Attachment {
        int ref = -1;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    };
    // 一个最小的子通道描述（可扩展到多输入/多颜色/深度/Resolve 等）
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

        // 在命令缓冲中开启/结束一个 RenderPass 实例
        void Begin (vk::raii::CommandBuffer& commandBuffer, TEVKFrameBuffer& frameBuffer, const std::vector<vk::ClearValue>& clearValue) const;
        void End (vk::raii::CommandBuffer& commandBuffer) const;
    private:
        vk::raii::RenderPass m_handle{VK_NULL_HANDLE};
        TEVKLogicDevice& m_logicDevice;
        RenderSubPass m_renderSubPass;
        vk::AttachmentDescription m_attachment;
    };
}