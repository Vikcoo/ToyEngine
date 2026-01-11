//
// Created by yukai on 2025/12/30.
//

#include "Resources/VulkanTexture2D.h"

#include "Texture2D.h"
#include "Core/VulkanDevice.h"
#include "VulkanBuffer.h"
#include "Commands/VulkanCommandPool.h"
#include "Commands/VulkanCommandBuffer.h"
#include "Core/VulkanQueue.h"
#include "Log/Log.h"
#include "AssetLoader.h"
#include <cmath>
#include <algorithm>


namespace TE
{


std::unique_ptr<VulkanTexture2D> VulkanTexture2D::CreateFromFile(const std::shared_ptr<VulkanDevice>& device, const std::string& path)
{
    auto rawData = AssetLoader::Load<RawTextureData>(path);

    auto tex = CreateFromData(device, rawData, vk::Format::eR8G8B8A8Srgb);

    return tex;
}

std::unique_ptr<VulkanTexture2D> VulkanTexture2D::CreateFromData(std::shared_ptr<VulkanDevice> device, std::shared_ptr<RawTextureData> rawTextureData , vk::Format format)
{
    auto texture = std::make_unique<VulkanTexture2D>();
    texture->m_width = rawTextureData->m_width;
    texture->m_height = rawTextureData->m_height;
    texture->m_channels = rawTextureData->m_channelCount;
    texture->m_format = format;
    
    // 计算 mipmap 层级数
    if (rawTextureData->m_needMipmap) {
        texture->m_mipLevelCount = rawTextureData->m_mipLevelCount;
    } else {
        texture->m_mipLevelCount = 1;
    }
    
    const vk::DeviceSize imageSize = rawTextureData->m_dataSize;


    // 创建像素数据的副本（因为原始数据可能被其他地方使用）
    std::vector<uint8_t> pixelDataCopy = rawTextureData->m_pixelData;

    // 翻转 Y 坐标
    FlipImageVerticallyInPlace(pixelDataCopy,
                               rawTextureData->m_width,
                               rawTextureData->m_height,
                               rawTextureData->m_channelCount);

    // 2. 创建 Staging Buffer（临时缓冲区，用于上传）
    BufferConfig stagingConfig;
    stagingConfig.size = imageSize;
    stagingConfig.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingConfig.memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    // 上传翻转后的数据
    auto stagingBuffer = device->CreateBuffer(stagingConfig);
    stagingBuffer->UploadData(pixelDataCopy.data(), imageSize);

    // 3. 创建图像（支持 mipmap）
    VulkanImageConfig imageConfig;
    imageConfig.width = rawTextureData->m_width;
    imageConfig.height = rawTextureData->m_height;
    imageConfig.format = format;
    imageConfig.usage = vk::ImageUsageFlagBits::eTransferSrc | 
                        vk::ImageUsageFlagBits::eTransferDst | 
                        vk::ImageUsageFlagBits::eSampled;
    imageConfig.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    imageConfig.tiling = vk::ImageTiling::eOptimal;
    imageConfig.mipLevelCount = texture->m_mipLevelCount;
    texture->m_image = device->CreateImage(imageConfig);


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
            vk::ImageLayout::eTransferDstOptimal,
            texture->m_mipLevelCount
        );

        // 6.2 从 Staging Buffer 复制到 Image（只复制 base mip level）
        CopyBufferToImage(
            cmdBuffer.get(),
            stagingBuffer->GetHandle(),
            texture->m_image->GetHandle(),
            texture->m_width,
            texture->m_height
        );

        // 6.3 生成 mipmap（如果启用）
        if (rawTextureData->m_needMipmap && texture->m_mipLevelCount > 1) {
            GenerateMipmaps(
                cmdBuffer.get(), 
                format, 
                texture->m_image->GetHandle(), 
                texture->m_width, 
                texture->m_height, 
                texture->m_mipLevelCount
            );
        } else {
            // 如果没有 mipmap，直接转换到 eShaderReadOnlyOptimal
            TransitionImageLayout(
                cmdBuffer.get(),
                texture->m_image->GetHandle(),
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                texture->m_mipLevelCount
            );
        }
    }
    // 7. 提交命令并等待完成
    auto graphicsQueue = device->GetGraphicsQueue();
    graphicsQueue->Submit(
        {cmdBuffer->GetRawHandle()},
        {}, {}, {}, nullptr
    );
    graphicsQueue->WaitIdle();

    // 8. 创建 ImageView
    VulkanImageViewConfig imageViewConfig;
    imageViewConfig.mipLevelCount = texture->m_mipLevelCount;
    imageViewConfig.format = format;
    imageViewConfig.image = texture->m_image->GetHandle();
    texture->m_imageView = std::make_unique<VulkanImageView>(
        VulkanImageView::PrivateTag{},
        device,
        imageViewConfig
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
    //samplerInfo.minLod = static_cast<float>(texture->m_mipLevelCount / 2); //0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(texture->m_mipLevelCount - 1);  // 使用实际的 mip level 数量

    texture->m_sampler = device->GetHandle().createSampler(samplerInfo);
    if (texture->m_sampler == nullptr)
    {
        TE_LOG_ERROR(" m_sampler failed");
    }

    return texture;
}

void VulkanTexture2D::TransitionImageLayout(
    const VulkanCommandBuffer* cmdBuffer,
    const vk::Image image,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const uint32_t mipLevels)
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
               mipLevels,  // levelCount
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
    } else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        // 支持从 eTransferSrcOptimal 转换（用于 mipmap 生成）
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
               .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
       TE_LOG_ERROR("Unsupported layout transition: {} -> {}", 
                    static_cast<int>(oldLayout), 
                    static_cast<int>(newLayout));
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

void VulkanTexture2D::FlipImageVerticallyInPlace(std::vector<uint8_t>& pixelData, uint32_t width, uint32_t height,
    uint32_t channels)
{
    const size_t rowSize = width * channels;
    const size_t halfHeight = height / 2;

    // 只交换上半部分和下半部分的行
    for (uint32_t y = 0; y < halfHeight; ++y) {
        const uint32_t srcRow = y;
        const uint32_t dstRow = height - 1 - y;

        uint8_t* srcPtr = pixelData.data() + srcRow * rowSize;
        uint8_t* dstPtr = pixelData.data() + dstRow * rowSize;

        // 交换两行
        for (size_t i = 0; i < rowSize; ++i) {
            std::swap(srcPtr[i], dstPtr[i]);
        }
    }
}

void VulkanTexture2D::GenerateMipmaps(const VulkanCommandBuffer* cmdBuffer, vk::Format imageFormat, vk::Image image, uint32_t width,
    uint32_t height, uint32_t mipLevels)
{
    // 检查格式是否支持线性过滤（mipmap 生成需要）
    const auto& physicalDevice = cmdBuffer->GetDevice()->GetPhysicalDevice();
    vk::FormatProperties formatProps = physicalDevice.GetHandle().getFormatProperties(imageFormat);
    
    if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        TE_LOG_ERROR("Texture image format does not support linear blitting!");
        return;
    }

    vk::ImageMemoryBarrier barrier;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = static_cast<int32_t>(width);
    int32_t mipHeight = static_cast<int32_t>(height);

    for (uint32_t i = 1; i < mipLevels; ++i) {
        // 将当前 mip level 从 eTransferDstOptimal 转换为 eTransferSrcOptimal
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmdBuffer->GetHandle().pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr,
            nullptr,
            barrier
        );

        // 执行 blit 操作：从 mip level i-1 到 mip level i
        vk::ImageBlit blit;
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        cmdBuffer->GetHandle().blitImage(
            image, 
            vk::ImageLayout::eTransferSrcOptimal,
            image, 
            vk::ImageLayout::eTransferDstOptimal,
            {blit},
            vk::Filter::eLinear
        );

        // 将当前 mip level 转换为 eShaderReadOnlyOptimal（已经 blit 完成）
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmdBuffer->GetHandle().pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            barrier
        );

        // 更新下一个 mip level 的尺寸
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // 转换最后一个 mip level（没有作为 src 使用，所以直接从 eTransferDstOptimal 转换）
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;  // 修复：应该是 Write
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmdBuffer->GetHandle().pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},
        nullptr,
        nullptr,
        barrier
    );
}
}


