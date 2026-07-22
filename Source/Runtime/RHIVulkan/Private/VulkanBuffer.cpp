// ToyEngine RHIVulkan Module
// Vulkan Buffer 实现

#include "VulkanBuffer.h"

#include "VulkanDevice.h"
#include "Log/Log.h"

#include <cstring>
#include <cstddef>

namespace TE {

namespace {

[[nodiscard]] VkBufferUsageFlags ToVulkanBufferUsage(const RHIBufferUsage usage)
{
    VkBufferUsageFlags result = 0;
    if (HasAnyFlags(usage, RHIBufferUsage::Vertex)) result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (HasAnyFlags(usage, RHIBufferUsage::Index)) result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (HasAnyFlags(usage, RHIBufferUsage::Uniform)) result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (HasAnyFlags(usage, RHIBufferUsage::Storage)) result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (HasAnyFlags(usage, RHIBufferUsage::CopySource) || HasAnyFlags(usage, RHIBufferUsage::Staging))
        result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (HasAnyFlags(usage, RHIBufferUsage::CopyDestination)) result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    return result;
}

} // namespace

VulkanBuffer::VulkanBuffer(VulkanDevice& device, const RHIBufferDesc& desc)
    : m_Device(&device)
    , m_Size(desc.size)
    , m_Usage(desc.usage)
    , m_MemoryUsage(desc.memoryUsage)
{
    if (desc.size == 0)
    {
        TE_LOG_ERROR("[RHIVulkan] Cannot create zero-sized buffer '{}'", desc.debugName);
        return;
    }

    VkBufferUsageFlags usage = ToVulkanBufferUsage(desc.usage);
    if (desc.initialData && desc.memoryUsage == RHIMemoryUsage::GPUOnly)
    {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (desc.memoryUsage == RHIMemoryUsage::CPUToGPU)
    {
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else if (desc.memoryUsage == RHIMemoryUsage::GPUToCPU)
    {
        // RHI 目前只有 UpdateData，没有 readback/invalidate 接口；阶段 B 先保证主机写入一致性。
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    if (!device.CreateBufferAllocation(desc.size, usage, properties, m_Buffer, m_Memory))
    {
        TE_LOG_ERROR("[RHIVulkan] Buffer allocation failed: {}", desc.debugName);
        return;
    }

    if ((properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
    {
        if (vkMapMemory(device.GetNativeDevice(), m_Memory, 0, desc.size, 0, &m_MappedData) != VK_SUCCESS)
        {
            TE_LOG_ERROR("[RHIVulkan] Buffer mapping failed: {}", desc.debugName);
            return;
        }
    }

    if (desc.initialData && !UpdateData(desc.initialData, desc.size, 0))
    {
        TE_LOG_ERROR("[RHIVulkan] Buffer initial upload failed: {}", desc.debugName);
        return;
    }
    m_Valid = true;
}

VulkanBuffer::~VulkanBuffer()
{
    if (!m_Device)
    {
        return;
    }
    const VkDevice device = m_Device->GetNativeDevice();
    if (m_MappedData)
    {
        vkUnmapMemory(device, m_Memory);
        m_MappedData = nullptr;
    }
    if (m_Buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, m_Buffer, nullptr);
    if (m_Memory != VK_NULL_HANDLE) vkFreeMemory(device, m_Memory, nullptr);
}

bool VulkanBuffer::UpdateData(const void* data, const uint64_t size, const uint64_t offset)
{
    if (!data || size == 0 || offset > m_Size || size > m_Size - offset || !m_Device ||
        m_Buffer == VK_NULL_HANDLE)
    {
        return false;
    }
    if (m_MappedData)
    {
        std::memcpy(static_cast<std::byte*>(m_MappedData) + offset, data, static_cast<size_t>(size));
        return true;
    }
    return m_Device->UploadBuffer(m_Buffer, offset, data, size);
}

} // namespace TE
