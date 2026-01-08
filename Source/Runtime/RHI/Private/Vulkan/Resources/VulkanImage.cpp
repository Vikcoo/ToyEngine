//
// Created by yukai on 2025/12/30.
//

#include "VulkanImage.h"

#include "../Core/VulkanDevice.h"
#include "Log/Log.h"

namespace TE
{
    VulkanImage::VulkanImage(PrivateTag, const std::shared_ptr<VulkanDevice>& device, const VulkanImageConfig& config):
    m_device(device), m_format(config.format)
    {
        vk::ImageCreateInfo imageCreateInfo {};
        imageCreateInfo.imageType = vk::ImageType::e2D;
        imageCreateInfo.extent.width = config.width;
        imageCreateInfo.extent.height = config.height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = config.mipLevelCount;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = config.format;
        imageCreateInfo.usage = config.usage;
        imageCreateInfo.tiling = config.tiling;
        imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;

        m_image = m_device->GetHandle().createImage(imageCreateInfo);
        if (m_image == VK_NULL_HANDLE)
        {
            TE_LOG_ERROR("Failed to create image!");
            return;
        }

        const vk::MemoryRequirements memRequirements = m_image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device->GetPhysicalDevice().FindMemoryType(memRequirements.memoryTypeBits, config.properties);

        m_memory = m_device->GetHandle().allocateMemory(allocInfo);
        if (m_memory == VK_NULL_HANDLE)
        {
            TE_LOG_ERROR("Failed to allocate memory!");
            return;
        }

        m_image.bindMemory(m_memory, 0);
    }

    std::unique_ptr<VulkanImage> VulkanImage::Create(std::shared_ptr<VulkanDevice> device, const VulkanImageConfig& config)
    {
        auto vulkanImage = std::make_unique<VulkanImage>(PrivateTag{}, device, config);
        return std::move(vulkanImage);
    }

   std::unique_ptr<VulkanImageView> VulkanImage::CreateImageView(vk::ImageAspectFlags aspectFlags)
    {
        return std::move(std::make_unique<VulkanImageView>(VulkanImageView::PrivateTag{}, m_device, m_image, m_format, aspectFlags));
    }
} // TE