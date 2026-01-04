// Vulkan Descriptor Pool 实现
#include "VulkanDescriptorPool.h"
#include "../Core/VulkanDevice.h"
#include "Log/Log.h"

namespace TE {

VulkanDescriptorPool::VulkanDescriptorPool(
    PrivateTag,
    std::shared_ptr<VulkanDevice> device,
    uint32_t maxSets,
    const std::vector<DescriptorPoolSize>& poolSizes,
    vk::DescriptorPoolCreateFlags flags)
    : m_device(std::move(device))
{
    if (poolSizes.empty()) {
        TE_LOG_ERROR("Descriptor pool sizes cannot be empty");
        throw std::runtime_error("Descriptor pool sizes cannot be empty");
    }

    // 转换为 Vulkan 格式
    std::vector<vk::DescriptorPoolSize> vkPoolSizes;
    vkPoolSizes.reserve(poolSizes.size());

    for (const auto& poolSize : poolSizes) {
        vk::DescriptorPoolSize vkPoolSize;
        vkPoolSize.setType(poolSize.type)
                   .setDescriptorCount(poolSize.count);
        vkPoolSizes.push_back(vkPoolSize);
    }

    // 创建描述符池
    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setMaxSets(maxSets)
              .setPoolSizes(vkPoolSizes)
              .setFlags(flags);

    try {
        m_pool = m_device->GetHandle().createDescriptorPool(createInfo);
        TE_LOG_DEBUG("Descriptor pool created: maxSets={}, poolSizeCount={}", 
                    maxSets, poolSizes.size());
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create descriptor pool: {}", e.what());
        throw;
    }
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
    TE_LOG_DEBUG("Descriptor pool destroyed");
}

std::vector<vk::raii::DescriptorSet> VulkanDescriptorPool::AllocateDescriptorSets(
    const std::vector<vk::DescriptorSetLayout>& layouts)
{
    if (layouts.empty()) {
        TE_LOG_ERROR("Cannot allocate descriptor sets: layouts is empty");
        return {};
    }

    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(*m_pool)
             .setSetLayouts(layouts);

    try {
        auto descriptorSets = m_device->GetHandle().allocateDescriptorSets(allocInfo);
        TE_LOG_DEBUG("Allocated {} descriptor set(s)", descriptorSets.size());
        return descriptorSets;
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to allocate descriptor sets: {}", e.what());
        throw;
    }
}

} // namespace TE

