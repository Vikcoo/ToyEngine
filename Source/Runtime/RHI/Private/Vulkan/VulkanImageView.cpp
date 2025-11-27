// Vulkan Image View 实现
#include "VulkanImageView.h"
#include "VulkanDevice.h"
#include "Log/Log.h"

namespace TE {

VulkanImageView::VulkanImageView(PrivateTag,
                                 std::shared_ptr<VulkanDevice> device,
                                 const vk::Image image,
                                 const vk::Format format,
                                 const vk::ImageAspectFlags aspectFlags)
    : m_device(std::move(device))
{
    vk::ImageViewCreateInfo createInfo;
    createInfo.setImage(image);
    createInfo.setViewType(vk::ImageViewType::e2D);
    createInfo.setFormat(format);
    
    vk::ComponentMapping components;
    components.setR(vk::ComponentSwizzle::eIdentity);
    components.setG(vk::ComponentSwizzle::eIdentity);
    components.setB(vk::ComponentSwizzle::eIdentity);
    components.setA(vk::ComponentSwizzle::eIdentity);
    createInfo.setComponents(components);
    
    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.setAspectMask(aspectFlags);
    subresourceRange.setBaseMipLevel(0);
    subresourceRange.setLevelCount(1);
    subresourceRange.setBaseArrayLayer(0);
    subresourceRange.setLayerCount(1);
    createInfo.setSubresourceRange(subresourceRange);

    try {
        m_imageView = m_device->GetHandle().createImageView(createInfo);
        TE_LOG_DEBUG("Image view created: Format={}", vk::to_string(format));
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create image view: {}", e.what());
        throw;
    }
}

VulkanImageView::~VulkanImageView() {
    TE_LOG_DEBUG("Image view destroyed");
}

} // namespace TE

