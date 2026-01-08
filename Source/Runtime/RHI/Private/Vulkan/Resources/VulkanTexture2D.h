//
// Created by yukai on 2025/12/30.
//

#pragma once
#include <memory>
#include <string>

#include "VulkanImage.h"
#include "vulkan/vulkan.hpp"
#include "VulkanImageView.h"


namespace TE
{
    struct RawTextureData;
    class VulkanDevice;
    class VulkanCommandBuffer;

class VulkanTexture2D
{
public:
    struct PrivateTag{
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };
    VulkanTexture2D() = default;
    static std::unique_ptr<VulkanTexture2D> CreateFromFile(const std::shared_ptr<VulkanDevice>& device, const std::string& path);

    static std::unique_ptr<VulkanTexture2D> CreateFromData(
        std::shared_ptr<VulkanDevice> device,
        std::shared_ptr<RawTextureData> rawTextureData,
        vk::Format format
    );
    static void TransitionImageLayout(const VulkanCommandBuffer* cmdBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);

    static void CopyBufferToImage(const VulkanCommandBuffer* cmdBuffer,vk::Buffer buffer,vk::Image image, uint32_t width, uint32_t height);

    static void FlipImageVerticallyInPlace(std::vector<uint8_t>& pixelData, uint32_t width, uint32_t height, uint32_t channels);

    static void GenerateMipmaps(const VulkanCommandBuffer* cmdBuffer, vk::Format imageFormat, vk::Image image, uint32_t width, uint32_t height, uint32_t mipLevels);

    // 获取 ImageView（用于描述符集）
    [[nodiscard]] const VulkanImageView& GetImageView() const { return *m_imageView; }

    // 获取 Sampler（用于描述符集）
    [[nodiscard]] const vk::raii::Sampler& GetSampler() const { return m_sampler; }
private:


    std::shared_ptr<VulkanDevice> m_device;
    std::unique_ptr<VulkanImage> m_image;
    std::unique_ptr<VulkanImageView> m_imageView;
    vk::raii::Sampler m_sampler{nullptr};
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    vk::Format m_format = vk::Format::eUndefined;
    uint32_t m_channels = 0;
    uint32_t m_mipLevelCount = 0;
};


}


