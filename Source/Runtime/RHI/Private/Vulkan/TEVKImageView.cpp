/*
文件用途: 图像视图实现
- 为指定 Image 创建 ImageView，确定视图类型/像素格式/子资源范围等
*/
#include "Vulkan/TEVKImageView.h"

namespace TE {
    // 构造: 基于图像句柄+格式+Aspect 创建 2D 视图（单层/单级）
    TEVKImageView::TEVKImageView(TEVKLogicDevice &logicDevice, vk::Image image, vk::Format format,
        vk::ImageAspectFlags aspectFlags):m_logicDevice(logicDevice) {
        vk::ImageViewCreateInfo createInfo;
        createInfo.setImage(image);
        createInfo.setViewType(vk::ImageViewType::e2D);
        createInfo.setFormat(format);
        createInfo.setComponents({vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity });
        createInfo.setSubresourceRange( vk::ImageSubresourceRange().setAspectMask(aspectFlags).setBaseMipLevel(0).setLevelCount(1).setBaseArrayLayer(0).setLayerCount(1));
        m_handle = m_logicDevice.GetHandle()->createImageView(createInfo);
    }
}