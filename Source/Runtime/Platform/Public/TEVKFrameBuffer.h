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
        TEVKFrameBuffer(TEVKLogicDevice* logicDevice, TEVKRenderPass* renderPass, const std::vector<vk::Image>& images,uint32_t width, uint32_t height);
        ~TEVKFrameBuffer();
        bool ReCreate(const std::vector<vk::Image>& images,uint32_t width, uint32_t height);
        private:
        vk::Framebuffer m_handle;
        TEVKLogicDevice* m_logicDevice;
        TEVKRenderPass* m_renderPass;
        uint32_t m_width;
        uint32_t m_height;
        std::vector<std::shared_ptr<TEVKImageView>> m_imageViews;
    };
}
