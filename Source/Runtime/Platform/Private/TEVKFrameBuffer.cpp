//
// Created by yukai on 2025/8/18.
//
#include "TEVKFrameBuffer.h"

#include "TEVKImageView.h"
#include "TEVKLogicDevice.h"
#include "TEVKRenderPass.h"

namespace TE {
    TEVKFrameBuffer::TEVKFrameBuffer(TEVKLogicDevice *logicDevice, TEVKRenderPass *renderPass,
        const std::vector<vk::Image> &images, uint32_t width, uint32_t height)
            :m_logicDevice(logicDevice), m_renderPass(renderPass), m_width(width), m_height(height){
        ReCreate(images,width,height);
    }

    TEVKFrameBuffer::~TEVKFrameBuffer() {
        m_logicDevice->GetHandle()->destroyFramebuffer(m_handle);
    }

    bool TEVKFrameBuffer::ReCreate(const std::vector<vk::Image> &images, uint32_t width, uint32_t height) {
        m_width = width;
        m_height = height;
        m_imageViews.clear();

        std::vector<vk::ImageView> imageViews;
        for (int i = 0; i < images.size(); ++i) {
            m_imageViews.push_back(std::make_shared<TEVKImageView>(m_logicDevice, images[i], m_logicDevice->GetSetting().format, vk::ImageAspectFlagBits::eColor ));
            imageViews.push_back(m_imageViews[i]->GetHandle());
        }



        m_logicDevice->GetHandle()->destroyFramebuffer(m_handle);


        vk::FramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.setRenderPass(m_renderPass->GetHandle());
        framebufferCreateInfo.setAttachmentCount(m_imageViews.size());
        framebufferCreateInfo.setAttachments(imageViews);
        framebufferCreateInfo.setWidth(m_width);
        framebufferCreateInfo.setHeight(m_height);
        framebufferCreateInfo.setLayers(1);
        m_handle = m_logicDevice->GetHandle()->createFramebuffer(framebufferCreateInfo);

        LOG_TRACE("framebuffer created,func:{}, framebuffer:{}, width:{}, height:{}, image view size :{}",__FUNCTION__, (void*) m_handle, m_width, m_height, m_imageViews.size());
        return true;
    }
}
