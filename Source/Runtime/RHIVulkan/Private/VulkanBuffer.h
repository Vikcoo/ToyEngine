// ToyEngine RHIVulkan Module
// Vulkan Buffer 与独立内存分配

#pragma once

#include "RHIBuffer.h"

#include <vulkan/vulkan.h>

namespace TE {

class VulkanDevice;

class VulkanBuffer final : public RHIBuffer
{
public:
    VulkanBuffer(VulkanDevice& device, const RHIBufferDesc& desc);
    ~VulkanBuffer() override;

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    [[nodiscard]] bool IsValid() const { return m_Valid; }
    [[nodiscard]] uint64_t GetSize() const override { return m_Size; }
    [[nodiscard]] RHIBufferUsage GetUsage() const override { return m_Usage; }
    bool UpdateData(const void* data, uint64_t size, uint64_t offset) override;

    [[nodiscard]] VkBuffer GetHandle() const { return m_Buffer; }

private:
    VulkanDevice* m_Device = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    void* m_MappedData = nullptr;
    uint64_t m_Size = 0;
    RHIBufferUsage m_Usage = RHIBufferUsage::None;
    RHIMemoryUsage m_MemoryUsage = RHIMemoryUsage::GPUOnly;
    bool m_Valid = false;
};

} // namespace TE
