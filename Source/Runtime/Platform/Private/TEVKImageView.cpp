//
// Created by yukai on 2025/8/18.
//

#include "TEVKImageView.h"

namespace TE {
    TEVKImageView::TEVKImageView(TEVKLogicDevice *logicDevice, vk::Image image, vk::Format format,
        vk::ImageAspectFlags aspectFlags):m_logicDevice(logicDevice) {
        vk::ImageViewCreateInfo createInfo;
        createInfo.setImage(image);
        createInfo.setViewType(vk::ImageViewType::e2D);
        createInfo.setFormat(format);
        createInfo.setComponents({vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity });
        createInfo.setSubresourceRange( vk::ImageSubresourceRange().setAspectMask(aspectFlags).setBaseMipLevel(0).setLevelCount(1).setBaseArrayLayer(0).setLayerCount(1));
        m_handle = m_logicDevice->GetHandle()->createImageView(createInfo);
    }

    TEVKImageView::~TEVKImageView() {
        m_logicDevice->GetHandle()->destroyImageView(m_handle);
    }
}
