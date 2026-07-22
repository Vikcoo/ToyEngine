// ToyEngine RHIVulkan Module
// Vulkan Sampler

#pragma once

#include "RHISampler.h"

#include <vulkan/vulkan.h>

namespace TE {

class VulkanDevice;

class VulkanSampler final : public RHISampler
{
public:
    VulkanSampler(VulkanDevice& device, const RHISamplerDesc& desc);
    ~VulkanSampler() override;

    [[nodiscard]] bool IsValid() const override { return m_Sampler != VK_NULL_HANDLE; }
    [[nodiscard]] VkSampler GetHandle() const { return m_Sampler; }

private:
    VulkanDevice* m_Device = nullptr;
    VkSampler m_Sampler = VK_NULL_HANDLE;
};

} // namespace TE
