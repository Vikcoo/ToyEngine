// ToyEngine RHIVulkan Module
// Vulkan Descriptor 实现

#include "VulkanDescriptors.h"

#include "VulkanBuffer.h"
#include "VulkanConversions.h"
#include "VulkanDevice.h"
#include "VulkanSampler.h"
#include "VulkanTexture.h"
#include "Log/Log.h"

#include <algorithm>

namespace TE {

VulkanBindGroupLayout::VulkanBindGroupLayout(VulkanDevice& device, const RHIBindGroupLayoutDesc& desc)
    : m_Device(&device)
    , m_Desc(desc)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(desc.entries.size());
    for (const auto& entry : desc.entries)
    {
        if (std::ranges::any_of(bindings, [&entry](const auto& value) { return value.binding == entry.binding; }))
        {
            TE_LOG_ERROR("[RHIVulkan] Duplicate descriptor binding {} in {}", entry.binding, desc.debugName);
            return;
        }
        bindings.push_back({
            .binding = entry.binding,
            .descriptorType = ToVulkanDescriptorType(entry.type),
            .descriptorCount = 1,
            .stageFlags = ToVulkanShaderStages(entry.visibility),
        });
    }
    const VkDescriptorSetLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
    if (vkCreateDescriptorSetLayout(device.GetNativeDevice(), &createInfo, nullptr, &m_Layout) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] DescriptorSetLayout creation failed: {}", desc.debugName);
    }
}

VulkanBindGroupLayout::~VulkanBindGroupLayout()
{
    if (m_Device && m_Layout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device->GetNativeDevice(), m_Layout, nullptr);
    }
}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& device, const RHIPipelineLayoutDesc& desc)
    : m_Device(&device)
{
    uint32_t layoutCount = 0;
    for (const auto& entry : desc.bindGroupLayouts)
    {
        layoutCount = std::max(layoutCount, entry.groupIndex + 1);
    }
    std::vector<VkDescriptorSetLayout> layouts(layoutCount, VK_NULL_HANDLE);
    for (const auto& entry : desc.bindGroupLayouts)
    {
        if (!entry.layout || !entry.layout->IsValid() || layouts[entry.groupIndex] != VK_NULL_HANDLE)
        {
            TE_LOG_ERROR("[RHIVulkan] Invalid or duplicate set {} in {}", entry.groupIndex, desc.debugName);
            return;
        }
        layouts[entry.groupIndex] = static_cast<VulkanBindGroupLayout*>(entry.layout)->GetHandle();
    }
    for (auto& layout : layouts)
    {
        if (layout != VK_NULL_HANDLE) continue;
        const VkDescriptorSetLayoutCreateInfo emptyInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        };
        if (vkCreateDescriptorSetLayout(device.GetNativeDevice(), &emptyInfo, nullptr, &layout) != VK_SUCCESS)
        {
            TE_LOG_ERROR("[RHIVulkan] Empty DescriptorSetLayout creation failed: {}", desc.debugName);
            return;
        }
        m_EmptyLayouts.push_back(layout);
    }
    const VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    if (vkCreatePipelineLayout(device.GetNativeDevice(), &createInfo, nullptr, &m_Layout) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] PipelineLayout creation failed: {}", desc.debugName);
    }
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    if (!m_Device) return;
    const VkDevice device = m_Device->GetNativeDevice();
    if (m_Layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_Layout, nullptr);
    for (const VkDescriptorSetLayout layout : m_EmptyLayouts)
    {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }
}

VulkanBindGroup::VulkanBindGroup(VulkanDevice& device, const RHIBindGroupDesc& desc)
    : m_Device(&device)
{
    if (!desc.layout || !desc.layout->IsValid())
    {
        TE_LOG_ERROR("[RHIVulkan] BindGroup has invalid layout: {}", desc.debugName);
        return;
    }
    auto* layout = static_cast<VulkanBindGroupLayout*>(desc.layout);
    const VkDescriptorSetLayout nativeLayout = layout->GetHandle();
    const VkDescriptorSetAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = device.GetDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &nativeLayout,
    };
    if (vkAllocateDescriptorSets(device.GetNativeDevice(), &allocateInfo, &m_Set) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] DescriptorSet allocation failed: {}", desc.debugName);
        return;
    }
    const auto releaseInvalidSet = [&]()
    {
        vkFreeDescriptorSets(device.GetNativeDevice(), device.GetDescriptorPool(), 1, &m_Set);
        m_Set = VK_NULL_HANDLE;
    };

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> writes;
    bufferInfos.reserve(desc.entries.size());
    imageInfos.reserve(desc.entries.size());
    writes.reserve(desc.entries.size());
    for (const auto& entry : desc.entries)
    {
        VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_Set,
            .dstBinding = entry.binding,
            .descriptorCount = 1,
            .descriptorType = ToVulkanDescriptorType(entry.type),
        };
        if (entry.type == RHIBindingType::UniformBuffer || entry.type == RHIBindingType::DynamicUniformBuffer)
        {
            if (!entry.buffer)
            {
                TE_LOG_ERROR("[RHIVulkan] BindGroup buffer binding {} is null: {}", entry.binding, desc.debugName);
                releaseInvalidSet();
                return;
            }
            auto* buffer = static_cast<VulkanBuffer*>(entry.buffer);
            bufferInfos.push_back({buffer->GetHandle(), entry.bufferOffset,
                                   entry.bufferSize == 0 ? VK_WHOLE_SIZE : entry.bufferSize});
            write.pBufferInfo = &bufferInfos.back();
        }
        else
        {
            VkImageView imageView = VK_NULL_HANDLE;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkSampler sampler = VK_NULL_HANDLE;
            if (entry.texture)
            {
                auto* texture = static_cast<VulkanTexture*>(entry.texture);
                imageView = texture->GetImageView();
                imageLayout = texture->GetLayout();
            }
            if (entry.sampler)
            {
                sampler = static_cast<VulkanSampler*>(entry.sampler)->GetHandle();
            }
            if ((entry.type != RHIBindingType::Sampler && imageView == VK_NULL_HANDLE) || sampler == VK_NULL_HANDLE)
            {
                TE_LOG_ERROR("[RHIVulkan] BindGroup image/sampler binding {} is incomplete: {}",
                             entry.binding, desc.debugName);
                releaseInvalidSet();
                return;
            }
            imageInfos.push_back({sampler, imageView, imageLayout});
            write.pImageInfo = &imageInfos.back();
        }
        writes.push_back(write);
    }
    vkUpdateDescriptorSets(device.GetNativeDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

VulkanBindGroup::~VulkanBindGroup()
{
    if (m_Device && m_Set != VK_NULL_HANDLE)
    {
        vkFreeDescriptorSets(m_Device->GetNativeDevice(), m_Device->GetDescriptorPool(), 1, &m_Set);
    }
}

} // namespace TE
