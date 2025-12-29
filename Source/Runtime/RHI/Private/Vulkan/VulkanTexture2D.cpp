//
// Created by yukai on 2025/12/30.
//

#include "VulkanTexture2D.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanQueue.h"
#include "stb-master/stb_image.h"
#include "Log/Log.h"

namespace TE
{

std::unique_ptr<VulkanTexture2D> VulkanTexture2D::CreateFromFile(const std::shared_ptr<VulkanDevice>& device, const std::string& path)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(
        path.c_str(),
        &texWidth,
        &texHeight,
        &texChannels,
        STBI_rgb_alpha);
    if (!pixels){
        TE_LOG_ERROR("create texture2D failed");
        return nullptr;
    };


    auto tex = CreateFromData(device, pixels, texWidth, texHeight, STBI_rgb_alpha, vk::Format::eR8G8B8A8Srgb);

    stbi_image_free(pixels);

    return tex;
}

std::unique_ptr<VulkanTexture2D> VulkanTexture2D::CreateFromData(std::shared_ptr<VulkanDevice> device, const void* data,
    uint32_t width, uint32_t height, uint32_t channels, vk::Format format)
{
    auto texture = std::make_unique<VulkanTexture2D>();
    texture->m_height = height;
    texture->m_width = width;
    texture->m_format = format;

    // 1. 计算图像大小
    const vk::DeviceSize imageSize = width * height * 4;

    // 2. 创建 Staging Buffer（临时缓冲区，用于上传）
    BufferConfig stagingConfig;
    stagingConfig.size = imageSize;
    stagingConfig.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingConfig.memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    auto stagingBuffer = device->CreateBuffer(stagingConfig);
    stagingBuffer->UploadData(data, imageSize);

    // 3. 创建图像
    VulkanImageConfig imageConfig;
    imageConfig.width = width;
    imageConfig.height = height;
    imageConfig.format = format;
    imageConfig.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageConfig.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    imageConfig.tiling = vk::ImageTiling::eOptimal;
    texture->m_image = device->CreateImage(imageConfig);

    // 命令缓冲的记录与执行
    // 5. 创建临时命令池和命令缓冲区（用于传输操作）
    auto commandPool = device->CreateCommandPool(
        device->GetQueueFamilies().graphics.value(),
        vk::CommandPoolCreateFlagBits::eTransient
    );

    auto commandBuffers = commandPool->AllocateCommandBuffers(1);
    auto& cmdBuffer = commandBuffers[0];

    // 6. 录制命令：转换布局、复制数据、再次转换布局
    {
        auto recording = cmdBuffer->BeginRecording(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        );

        // 6.1 转换布局：eUndefined -> eTransferDstOptimal
        TransitionImageLayout(
            cmdBuffer.get(),
            texture->m_image->GetHandle(),  // 获取原始 vk::Image
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal
        );

        // 6.2 从 Staging Buffer 复制到 Image
        CopyBufferToImage(
            cmdBuffer.get(),
            stagingBuffer->GetHandle(),
            texture->m_image->GetHandle(),  // 获取原始 vk::Image
            width,
            height
        );

        // 6.3 转换布局：eTransferDstOptimal -> eShaderReadOnlyOptimal
        TransitionImageLayout(
            cmdBuffer.get(),
            texture->m_image->GetHandle(),
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
    }
    // 7. 提交命令并等待完成
    auto graphicsQueue = device->GetGraphicsQueue();
    graphicsQueue->Submit(
        {cmdBuffer->GetRawHandle()},
        {}, {}, {}, nullptr
    );
    graphicsQueue->WaitIdle();

    // 8. 创建 ImageView
    texture->m_imageView = std::make_unique<VulkanImageView>(
        VulkanImageView::PrivateTag{},
        device,
        texture->m_image->GetHandle(),
        format,
        vk::ImageAspectFlagBits::eColor
    );

    // 9 采样器

    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = true;

    const auto properties = device->GetPhysicalDevice().GetHandle().getProperties();
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = false;
    samplerInfo.compareEnable = false;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    texture->m_sampler = device->GetHandle().createSampler(samplerInfo);
    if (texture->m_sampler == nullptr)
    {
        TE_LOG_ERROR(" m_sampler failed");
    }

    return texture;
}

void VulkanTexture2D::TransitionImageLayout(const VulkanCommandBuffer* cmdBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(oldLayout)
           .setNewLayout(newLayout)
           .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
           .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
           .setImage(image)
           .setSubresourceRange({
               vk::ImageAspectFlagBits::eColor,
               0,  // baseMipLevel
               1,  // levelCount
               0,  // baseArrayLayer
               1   // layerCount
           });

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
               .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
               .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
       TE_LOG_ERROR("Unsupported layout transition!");
       return;
    }

    cmdBuffer->GetHandle().pipelineBarrier(
        sourceStage,
        destinationStage,
        {},
        {},
        {},
        {barrier}
    );
}

void VulkanTexture2D::CopyBufferToImage(const VulkanCommandBuffer* cmdBuffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
    vk::BufferImageCopy region;
    region.setBufferOffset(0)
          .setBufferRowLength(0)
          .setBufferImageHeight(0)
          .setImageSubresource({
              vk::ImageAspectFlagBits::eColor,
              0,  // mipLevel
              0,  // baseArrayLayer
              1   // layerCount
          })
          .setImageOffset({0, 0, 0})
          .setImageExtent({width, height, 1});

    cmdBuffer->GetHandle().copyBufferToImage(
        buffer,
        image,
        vk::ImageLayout::eTransferDstOptimal,
        {region}
    );
}
}


