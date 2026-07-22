// ToyEngine RHIVulkan Module
// Vulkan Sampler 实现

#include "VulkanSampler.h"

#include "VulkanDevice.h"
#include "Log/Log.h"

namespace TE {

namespace {

[[nodiscard]] VkFilter ToFilter(const RHITextureFilter filter)
{
    return filter == RHITextureFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

[[nodiscard]] VkSamplerAddressMode ToAddressMode(const RHITextureAddressMode mode)
{
    return mode == RHITextureAddressMode::ClampToEdge
        ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
        : VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

} // namespace

VulkanSampler::VulkanSampler(VulkanDevice& device, const RHISamplerDesc& desc)
    : m_Device(&device)
{
    const VkSamplerCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = ToFilter(desc.magFilter),
        .minFilter = ToFilter(desc.minFilter),
        .mipmapMode = desc.minFilter == RHITextureFilter::Nearest
            ? VK_SAMPLER_MIPMAP_MODE_NEAREST
            : VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = ToAddressMode(desc.addressU),
        .addressModeV = ToAddressMode(desc.addressV),
        .addressModeW = ToAddressMode(desc.addressW),
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
    };
    if (vkCreateSampler(device.GetNativeDevice(), &createInfo, nullptr, &m_Sampler) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] Sampler creation failed: {}", desc.debugName);
    }
}

VulkanSampler::~VulkanSampler()
{
    if (m_Device && m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device->GetNativeDevice(), m_Sampler, nullptr);
    }
}

} // namespace TE
