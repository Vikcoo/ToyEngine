// ToyEngine RHIVulkan Module
// Vulkan Image、ImageView 与资源状态

#pragma once

#include "RHITexture.h"

#include <vulkan/vulkan.h>

namespace TE {

class VulkanDevice;

class VulkanTexture final : public RHITexture
{
public:
    VulkanTexture(VulkanDevice& device, const RHITextureDesc& desc);
    ~VulkanTexture() override;

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    [[nodiscard]] bool IsValid() const override { return m_Valid; }
    [[nodiscard]] uint32_t GetWidth() const override { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const override { return m_Height; }
    [[nodiscard]] RHIFormat GetFormat() const override { return m_Format; }
    [[nodiscard]] RHITextureUsage GetUsage() const override { return m_Usage; }
    [[nodiscard]] RHISampleCount GetSampleCount() const override { return m_SampleCount; }

    [[nodiscard]] VkImage GetImage() const { return m_Image; }
    [[nodiscard]] VkImageView GetImageView() const { return m_ImageView; }
    [[nodiscard]] VkImageLayout GetLayout() const { return m_Layout; }
    [[nodiscard]] VkImageAspectFlags GetAspectMask() const { return m_AspectMask; }
    void RecordTransition(VkCommandBuffer commandBuffer, RHIResourceState before, RHIResourceState after);

private:
    VulkanDevice* m_Device = nullptr;
    VkImage m_Image = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkFormat m_NativeFormat = VK_FORMAT_UNDEFINED;
    VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags m_AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    RHIFormat m_Format = RHIFormat::Undefined;
    RHITextureUsage m_Usage = RHITextureUsage::None;
    RHISampleCount m_SampleCount = RHISampleCount::Count1;
    bool m_Valid = false;
};

} // namespace TE
