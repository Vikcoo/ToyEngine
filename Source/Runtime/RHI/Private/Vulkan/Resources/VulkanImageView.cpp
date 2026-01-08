// Vulkan Image View 实现
#include "VulkanImageView.h"
#include "Core/VulkanDevice.h"
#include "Log/Log.h"

namespace TE {

VulkanImageView::VulkanImageView(PrivateTag, std::shared_ptr<VulkanDevice> device,const VulkanImageViewConfig& config)
    : m_device(std::move(device))
{
    vk::ComponentMapping components;
    components.setR(vk::ComponentSwizzle::eIdentity)
              .setG(vk::ComponentSwizzle::eIdentity)
              .setB(vk::ComponentSwizzle::eIdentity)
              .setA(vk::ComponentSwizzle::eIdentity);
    
    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.setAspectMask(config.aspectFlags)
                     .setBaseMipLevel(0)
                     .setLevelCount(1)
                     .setBaseArrayLayer(0)
                     .setLayerCount(1);
    
    vk::ImageViewCreateInfo createInfo;
    createInfo.setImage(config.image->GetHandle())
              .setViewType(vk::ImageViewType::e2D)
              .setFormat(config.format)
              .setComponents(components)
              .setSubresourceRange(subresourceRange);


    m_imageView = m_device->GetHandle().createImageView(createInfo);
    if (m_imageView == nullptr)
    {
        TE_LOG_ERROR("Failed to create image view: {}");
    }
    TE_LOG_DEBUG("Image view created: Format={}", vk::to_string(config.format));
}

VulkanImageView::~VulkanImageView() {
    TE_LOG_DEBUG("Image view destroyed");
}

} // namespace TE

