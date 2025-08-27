//
// Created by yukai on 2025/8/18.
//
#pragma once
#include <vector>
#include "TEVKCommon.h"

namespace TE {
    class TEVKLogicDevice;
    class TEVKRenderPass;
    class TEVKImageView;
    class TEVKFrameBuffer {
        public:
        TEVKFrameBuffer(TEVKLogicDevice& logicDevice, TEVKRenderPass& renderPass, const std::vector<vk::Image>& images,uint32_t width, uint32_t height);
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
