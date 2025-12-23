// Vulkan Descriptor Set Layout 实现
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDevice.h"
#include "Log/Log.h"
#include <algorithm>

namespace TE {

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    PrivateTag,
    std::shared_ptr<VulkanDevice> device,
    const std::vector<DescriptorSetLayoutBinding>& bindings)
    : m_device(std::move(device))
{
    if (bindings.empty()) {
        TE_LOG_ERROR("Descriptor set layout bindings cannot be empty");
        throw std::runtime_error("Descriptor set layout bindings cannot be empty");
    }

    // 转换为 Vulkan 格式
    std::vector<vk::DescriptorSetLayoutBinding> vkBindings;
    vkBindings.reserve(bindings.size());

    for (const auto& binding : bindings) {
        vk::DescriptorSetLayoutBinding vkBinding;
        vkBinding.setBinding(binding.binding)
                  .setDescriptorType(binding.descriptorType)
                  .setDescriptorCount(binding.descriptorCount)
                  .setStageFlags(binding.stageFlags)
                  .setPImmutableSamplers(nullptr);  // 对于 UBO，不需要采样器

        vkBindings.push_back(vkBinding);
    }

    // 创建描述符集布局
    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.setBindings(vkBindings);

    try {
        m_layout = m_device->GetHandle().createDescriptorSetLayout(createInfo);
        TE_LOG_DEBUG("Descriptor set layout created with {} binding(s)", bindings.size());
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create descriptor set layout: {}", e.what());
        throw;
    }
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
    TE_LOG_DEBUG("Descriptor set layout destroyed");
}

} // namespace TE

