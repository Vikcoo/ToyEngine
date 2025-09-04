/* 
文件用途: 帧缓冲实现
- 将一组图像视图(ImageView)与 RenderPass 绑定成 Framebuffer
- 常用于配合交换链图像重建窗口大小适配
*/
//
#include "Vulkan/TEVKFrameBuffer.h"

#include "Vulkan/TEVKImageView.h"
#include "Vulkan/TEVKLogicDevice.h"
#include "Vulkan/TEVKRenderPass.h"

namespace TE {
    // 构造: 保存引用并立即创建 Framebuffer（为每张图像创建 ImageView）
    TEVKFrameBuffer::TEVKFrameBuffer(TEVKLogicDevice &logicDevice, TEVKRenderPass &renderPass,
        const std::vector<vk::Image> &images, uint32_t width, uint32_t height)
            :m_logicDevice(logicDevice), m_renderPass(renderPass), m_width(width), m_height(height){
        ReCreate(images,width,height);
    }

    // 重建: 基于新的图像与尺寸，重建内部 ImageView 列表与 Framebuffer
    bool TEVKFrameBuffer::ReCreate(const std::vector<vk::Image> &images, uint32_t width, uint32_t height) {
        m_width = width;
        m_height = height;

        m_imageViews.clear();

        std::vector<vk::ImageView> imageViews;
        for (int i = 0; i < images.size(); ++i) {
            m_imageViews.push_back(std::make_shared<TEVKImageView>(m_logicDevice, images[i], m_logicDevice.GetSetting().format, vk::ImageAspectFlagBits::eColor ));
            imageViews.push_back(m_imageViews[i]->GetHandle());
        }

        vk::FramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.setRenderPass(m_renderPass.GetHandle());
        framebufferCreateInfo.setAttachments(imageViews);
        framebufferCreateInfo.setWidth(m_width);
        framebufferCreateInfo.setHeight(m_height);
        framebufferCreateInfo.setLayers(1);
        m_handle = m_logicDevice.GetHandle()->createFramebuffer(framebufferCreateInfo);

        LOG_TRACE("帧缓冲创建完成, func:{}, framebuffer:{}, 宽:{}, 高:{}, image view数量 :{}",__FUNCTION__, reinterpret_cast<uint64_t>(static_cast<void*>(*m_handle)), m_width, m_height, m_imageViews.size());
        return true;
    }
}