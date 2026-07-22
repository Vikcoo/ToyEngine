// ToyEngine RHIVulkan Module
// Vulkan Graphics Pipeline

#pragma once

#include "RHIPipeline.h"

#include <vulkan/vulkan.h>

namespace TE {

class VulkanDevice;
class VulkanPipelineLayout;

class VulkanPipeline final : public RHIPipeline
{
public:
    VulkanPipeline(VulkanDevice& device, const RHIPipelineDesc& desc);
    ~VulkanPipeline() override;

    [[nodiscard]] bool IsValid() const override { return m_Pipeline != VK_NULL_HANDLE; }
    [[nodiscard]] VkPipeline GetHandle() const { return m_Pipeline; }
    [[nodiscard]] const VulkanPipelineLayout* GetLayout() const { return m_Layout; }

private:
    VulkanDevice* m_Device = nullptr;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    const VulkanPipelineLayout* m_Layout = nullptr;
};

} // namespace TE
