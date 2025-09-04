/*
文件用途: 命令池/命令缓冲实现
- 创建与分配 CommandBuffer；提供 Begin/End 辅助，便于录制命令
- 概念: CommandPool 绑定到某个队列族；CommandBuffer 需先 begin 再记录再 end
*/
#include "Vulkan/TEVKCommandBuffer.h"
#include "Vulkan/TEVKLogicDevice.h"

namespace TE{
    // 构造: 基于队列族创建 CommandPool，启用 eResetCommandBuffer 以便单独重置
    TEVKCommandPool::TEVKCommandPool(TEVKLogicDevice &device, uint32_t queueFamilyIndex) : m_device(device) {
        vk::CommandPoolCreateInfo commandPoolInfo;
        commandPoolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        commandPoolInfo.setQueueFamilyIndex(queueFamilyIndex);

        m_handle = m_device.GetHandle()->createCommandPool(commandPoolInfo);
        LOG_TRACE("Create command pool : {0}", reinterpret_cast<uint64_t>(static_cast<void*>(*m_handle)));
    }

    // 分配指定数量的 Primary 级别命令缓冲
    std::vector<vk::raii::CommandBuffer> TEVKCommandPool::AllocateCommandBuffer(uint32_t count) const {
        vk::CommandBufferAllocateInfo allocateInfo ;
        allocateInfo.setCommandPool(m_handle);
        allocateInfo.setCommandBufferCount(count);
        allocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        return m_device.GetHandle()->allocateCommandBuffers(allocateInfo);
    }

    // 开始录制命令缓冲：重置后调用 begin
    void TEVKCommandPool::BeginCommandBuffer(vk::raii::CommandBuffer& cmdBuffer) {
        cmdBuffer.reset();
        vk::CommandBufferBeginInfo beginInfo;
        cmdBuffer.begin(beginInfo);
    }

    // 结束录制命令缓冲
    void TEVKCommandPool::EndCommandBuffer(vk::raii::CommandBuffer& cmdBuffer) {
        cmdBuffer.end();
    }
}