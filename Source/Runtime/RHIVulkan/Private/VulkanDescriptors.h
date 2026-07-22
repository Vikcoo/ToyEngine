// ToyEngine RHIVulkan Module
// Vulkan DescriptorSetLayout、PipelineLayout 与 DescriptorSet

#pragma once

#include "RHIBindGroup.h"
#include "RHIPipeline.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace TE {

class VulkanDevice;

class VulkanBindGroupLayout final : public RHIBindGroupLayout
{
public:
    VulkanBindGroupLayout(VulkanDevice& device, const RHIBindGroupLayoutDesc& desc);
    ~VulkanBindGroupLayout() override;

    [[nodiscard]] bool IsValid() const override { return m_Layout != VK_NULL_HANDLE; }
    [[nodiscard]] VkDescriptorSetLayout GetHandle() const { return m_Layout; }
    [[nodiscard]] const RHIBindGroupLayoutDesc& GetDesc() const { return m_Desc; }

private:
    VulkanDevice* m_Device = nullptr;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    RHIBindGroupLayoutDesc m_Desc;
};

class VulkanPipelineLayout final : public RHIPipelineLayout
{
public:
    VulkanPipelineLayout(VulkanDevice& device, const RHIPipelineLayoutDesc& desc);
    ~VulkanPipelineLayout() override;

    [[nodiscard]] bool IsValid() const override { return m_Layout != VK_NULL_HANDLE; }
    [[nodiscard]] VkPipelineLayout GetHandle() const { return m_Layout; }

private:
    VulkanDevice* m_Device = nullptr;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> m_EmptyLayouts;
};

class VulkanBindGroup final : public RHIBindGroup
{
public:
    VulkanBindGroup(VulkanDevice& device, const RHIBindGroupDesc& desc);
    ~VulkanBindGroup() override;

    [[nodiscard]] bool IsValid() const override { return m_Set != VK_NULL_HANDLE; }
    [[nodiscard]] VkDescriptorSet GetHandle() const { return m_Set; }

private:
    VulkanDevice* m_Device = nullptr;
    VkDescriptorSet m_Set = VK_NULL_HANDLE;
};

} // namespace TE
