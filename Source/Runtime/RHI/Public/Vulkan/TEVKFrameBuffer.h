/*
文件用途: 帧缓冲封装
- 负责: 为渲染通道(RenderPass)与目标图像(ImageView)组装 Framebuffer
- 概念: Framebuffer = RenderPass 在某一组具体附件(attachments)上的实例
*/
#pragma once
#include <vector>
#include "TEVKCommon.h"

namespace TE {
    class TEVKLogicDevice;
    class TEVKRenderPass;
    class TEVKImageView;

    // 帧缓冲对象：与 RenderPass/附件图像尺寸保持一致
    class TEVKFrameBuffer {
        public:
        TEVKFrameBuffer(TEVKLogicDevice& logicDevice, TEVKRenderPass& renderPass, const std::vector<vk::Image>& images,uint32_t width, uint32_t height);
        // 在交换链重建或尺寸变化时，重建内部 ImageView/Framebuffer
        bool ReCreate(const std::vector<vk::Image>& images,uint32_t width, uint32_t height);
        const vk::raii::Framebuffer& GetHandle() { return m_handle;}
        const uint32_t GetWidth() const{ return m_width;}
        const uint32_t GetHeight() const{ return m_height;}
        private:
        vk::raii::Framebuffer m_handle{ nullptr };
        TEVKLogicDevice& m_logicDevice;
        TEVKRenderPass& m_renderPass;
        uint32_t m_width;
        uint32_t m_height;
        std::vector<std::shared_ptr<TEVKImageView>> m_imageViews;
    };
}