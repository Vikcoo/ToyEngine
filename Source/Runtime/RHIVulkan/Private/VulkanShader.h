// ToyEngine RHIVulkan Module
// Vulkan ShaderModule 与逻辑资产解析

#pragma once

#include "RHIShader.h"

#include <vulkan/vulkan.h>

#include <string>

namespace TE {

class VulkanDevice;

class VulkanShader final : public RHIShader
{
public:
    VulkanShader(VulkanDevice& device, const RHIShaderDesc& desc);
    ~VulkanShader() override;

    [[nodiscard]] RHIShaderStage GetStage() const override { return m_Stage; }
    [[nodiscard]] bool IsValid() const override { return m_Module != VK_NULL_HANDLE; }
    [[nodiscard]] VkShaderModule GetHandle() const { return m_Module; }
    [[nodiscard]] const std::string& GetEntryPoint() const { return m_EntryPoint; }

private:
    VulkanDevice* m_Device = nullptr;
    VkShaderModule m_Module = VK_NULL_HANDLE;
    RHIShaderStage m_Stage = RHIShaderStage::Vertex;
    std::string m_EntryPoint;
};

} // namespace TE
