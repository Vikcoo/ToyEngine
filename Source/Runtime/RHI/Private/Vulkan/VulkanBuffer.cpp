// Vulkan Buffer 实现
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "Log/Log.h"
#include <cstring>

namespace TE {

VulkanBuffer::VulkanBuffer(PrivateTag,
                           std::shared_ptr<VulkanDevice> device,
                           const BufferConfig& config)
    : m_device(std::move(device))
    , m_size(config.size)
{
    if (m_size == 0) {
        TE_LOG_ERROR("Buffer size cannot be zero");
        throw std::runtime_error("Buffer size cannot be zero");
    }

    // 创建缓冲区
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(m_size)
              .setUsage(config.usage)
              .setSharingMode(vk::SharingMode::eExclusive);  // 独占模式

    try {
        m_buffer = m_device->GetHandle().createBuffer(bufferInfo);
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create buffer: {}", e.what());
        throw;
    }

    // 获取内存需求
    vk::MemoryRequirements memRequirements = m_buffer.getMemoryRequirements();

    // 分配内存
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.setAllocationSize(memRequirements.size)
             .setMemoryTypeIndex(
                 m_device->GetPhysicalDevice().FindMemoryType(
                 memRequirements.memoryTypeBits,
                 config.memoryProperties
             ));

    try {
        m_memory = m_device->GetHandle().allocateMemory(allocInfo);
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to allocate buffer memory: {}", e.what());
        throw;
    }

    // 绑定内存到缓冲区
    m_buffer.bindMemory(m_memory, 0);

    TE_LOG_DEBUG("Buffer created: size={} bytes", m_size);
}

VulkanBuffer::~VulkanBuffer() {
    // RAII 自动清理
    TE_LOG_DEBUG("Buffer destroyed");
}

void VulkanBuffer::UploadData(const void* data, size_t size, size_t offset) {
    if (!data) {
        TE_LOG_ERROR("Cannot upload null data");
        return;
    }

    // 如果 size 为 0，使用配置中的大小
    size_t uploadSize = (size == 0) ? m_size : size;
    
    // 检查边界
    if (offset + uploadSize > m_size) {
        TE_LOG_ERROR("Upload size exceeds buffer capacity: offset={}, size={}, capacity={}", 
                    offset, uploadSize, m_size);
        return;
    }

    // 映射内存
    void* mappedMemory = m_memory.mapMemory(0, uploadSize);
    if (!mappedMemory) {
        TE_LOG_ERROR("Failed to map buffer memory");
        return;
    }

    // 复制数据
    memcpy(mappedMemory, data, uploadSize);

    // 如果内存不是主机一致的，需要手动刷新
    // 但因为我们默认使用 eHostCoherent，所以不需要刷新
    // 如果将来需要支持非一致内存，可以在这里添加刷新逻辑

    // 取消映射
    m_memory.unmapMemory();
}



} // namespace TE

