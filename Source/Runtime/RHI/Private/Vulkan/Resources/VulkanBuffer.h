// Vulkan Buffer - 缓冲区
#pragma once

#include "../Core/VulkanDevice.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace TE {

class VulkanDevice;

/// 缓冲区配置
struct BufferConfig {
    size_t size;  // 缓冲区大小（字节）
    vk::BufferUsageFlags usage;  // 缓冲区用途（如 eVertexBuffer, eIndexBuffer）
    vk::MemoryPropertyFlags memoryProperties = 
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent;  // 内存属性（默认主机可见且一致）
};

/// Vulkan 缓冲区 - 管理顶点缓冲区、索引缓冲区等
class VulkanBuffer {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    explicit VulkanBuffer(PrivateTag,
                         std::shared_ptr<VulkanDevice> device,
                         const BufferConfig& config);

    ~VulkanBuffer();

    // 禁用拷贝，允许移动
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&&) noexcept = default;
    VulkanBuffer& operator=(VulkanBuffer&&) noexcept = default;

    // 上传数据到缓冲区
    // data: 要上传的数据指针
    // size: 数据大小（字节），如果为0则使用配置中的size
    // offset: 缓冲区中的偏移量（字节）
    void UploadData(const void* data, size_t size = 0, size_t offset = 0);

    // 获取缓冲区句柄
    [[nodiscard]] vk::Buffer GetHandle() const { return *m_buffer; }
    [[nodiscard]] const vk::raii::Buffer& GetRAIIHandle() const { return m_buffer; }
    [[nodiscard]] size_t GetSize() const { return m_size; }



private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::Buffer m_buffer{nullptr};
    vk::raii::DeviceMemory m_memory{nullptr};
    size_t m_size;
};

} // namespace TE

