#include "Vulkan/TEVKCommandBuffer.h"
#include "Vulkan/TEVKLogicDevice.h"

namespace TE{
    TEVKCommandPool::TEVKCommandPool(TEVKLogicDevice &device, uint32_t queueFamilyIndex) : m_device(device) {
        vk::CommandPoolCreateInfo commandPoolInfo;
        commandPoolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPoolInfo.setQueueFamilyIndex(queueFamilyIndex);

        m_handle = m_device.GetHandle()->createCommandPool(commandPoolInfo);
        LOG_TRACE("Create command pool : {0}", reinterpret_cast<uint64_t>(static_cast<void*>(*m_handle)));
    }

    std::vector<vk::raii::CommandBuffer> TEVKCommandPool::AllocateCommandBuffer(uint32_t count) const {
        vk::CommandBufferAllocateInfo allocateInfo ;
        allocateInfo.setCommandPool(m_handle);
        allocateInfo.setCommandBufferCount(count);
        allocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        return m_device.GetHandle()->allocateCommandBuffers(allocateInfo);
    }

    void TEVKCommandPool::BeginCommandBuffer(vk::raii::CommandBuffer& cmdBuffer) {
        cmdBuffer.reset();
        vk::CommandBufferBeginInfo beginInfo;
        cmdBuffer.begin(beginInfo);
    }

    void TEVKCommandPool::EndCommandBuffer(vk::raii::CommandBuffer& cmdBuffer) {
        cmdBuffer.end();
    }
}