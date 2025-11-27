// Vulkan Command Pool 实现
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"
#include "Log/Log.h"

namespace TE {

VulkanCommandPool::VulkanCommandPool(PrivateTag,
                                     std::shared_ptr<VulkanDevice> device,
                                     const uint32_t queueFamilyIndex,
                                     const vk::CommandPoolCreateFlags flags)
    : m_device(std::move(device))
    , m_queueFamilyIndex(queueFamilyIndex)
{
    vk::CommandPoolCreateInfo createInfo;
    createInfo.setQueueFamilyIndex(queueFamilyIndex);
    createInfo.setFlags(flags);

    try {
        m_pool = m_device->GetHandle().createCommandPool(createInfo);
        TE_LOG_DEBUG("Command pool created: QueueFamily={}", queueFamilyIndex);
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create command pool: {}", e.what());
        throw;
    }
}

VulkanCommandPool::~VulkanCommandPool() {
    TE_LOG_DEBUG("Command pool destroyed");
}

std::vector<vk::raii::CommandBuffer> VulkanCommandPool::AllocateBuffers(
    const uint32_t count,
    const vk::CommandBufferLevel level)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(*m_pool);
    allocInfo.setLevel(level);
    allocInfo.setCommandBufferCount(count);

    try {
        auto buffers = vk::raii::CommandBuffers(m_device->GetHandle(), allocInfo);
        TE_LOG_DEBUG("Allocated {} command buffer(s)", count);
        
        // 转换为 vector
        std::vector<vk::raii::CommandBuffer> result;
        result.reserve(count);
        for (auto& buffer : buffers) {
            result.push_back(std::move(buffer));
        }
        return result;
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to allocate command buffers: {}", e.what());
        throw;
    }
}

void VulkanCommandPool::Reset(const vk::CommandPoolResetFlags flags) {
    try {
        m_pool.reset(flags);
        TE_LOG_DEBUG("Command pool reset");
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to reset command pool: {}", e.what());
        throw;
    }
}

} // namespace TE


