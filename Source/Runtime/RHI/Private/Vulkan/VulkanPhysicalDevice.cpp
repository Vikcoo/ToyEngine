// Vulkan Physical Device 实现
#include "VulkanPhysicalDevice.h"
#include "VulkanContext.h"
#include "VulkanSurface.h"
#include "VulkanUtils.h"
#include "Log/Log.h"

namespace TE {

// ============================================================================
// 构造函数
// ============================================================================

VulkanPhysicalDevice::VulkanPhysicalDevice(PrivateTag, std::weak_ptr<VulkanContext> context,
                                           vk::raii::PhysicalDevice device)
    : m_context(std::move(context))
    , m_device(std::move(device))
{
    TE_LOG_DEBUG("Physical Device: {}", GetDeviceName());
}

// ============================================================================
// 属性查询（带缓存）
// ============================================================================

vk::PhysicalDeviceProperties VulkanPhysicalDevice::GetProperties() const {
    if (!m_cachedProperties.has_value()) {
        m_cachedProperties = m_device.getProperties();
    }
    return m_cachedProperties.value();
}

vk::PhysicalDeviceFeatures VulkanPhysicalDevice::GetFeatures() const {
    if (!m_cachedFeatures.has_value()) {
        m_cachedFeatures = m_device.getFeatures();
    }
    return m_cachedFeatures.value();
}

vk::PhysicalDeviceMemoryProperties VulkanPhysicalDevice::GetMemoryProperties() const {
    return m_device.getMemoryProperties();
}

std::vector<vk::QueueFamilyProperties> VulkanPhysicalDevice::GetQueueFamilies() const {
    if (!m_cachedQueueFamilies.has_value()) {
        m_cachedQueueFamilies = m_device.getQueueFamilyProperties();
    }
    return m_cachedQueueFamilies.value();
}

std::string VulkanPhysicalDevice::GetDeviceName() const {
    const auto props = GetProperties();
    return props.deviceName.data();
}

// ============================================================================
// 队列族查找
// ============================================================================

std::optional<uint32_t> VulkanPhysicalDevice::FindQueueFamily(
    const vk::QueueFlags queueFlags,
    const VulkanSurface* presentSurface) const
{
    const auto queueFamilies = GetQueueFamilies();
    
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        const auto& queueFamily = queueFamilies[i];
        
        // 检查队列标志
        const bool flagsMatch = (queueFamily.queueFlags & queueFlags) == queueFlags;
        
        // 检查呈现支持
        bool presentSupport = true;
        if (presentSurface != nullptr) {
            const VkBool32 supported = m_device.getSurfaceSupportKHR(i, *presentSurface->GetHandle());
            presentSupport = (supported == VK_TRUE);
        }
        
        if (flagsMatch && presentSupport && queueFamily.queueCount > 0) {
            return i;
        }
    }
    
    return std::nullopt;
}

QueueFamilyIndices VulkanPhysicalDevice::FindQueueFamilies(const VulkanSurface* presentSurface) const {
    QueueFamilyIndices indices;
    const auto queueFamilies = GetQueueFamilies();
    
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        const auto& queueFamily = queueFamilies[i];
        
        // 图形队列
        if (!indices.graphics.has_value() && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
            indices.graphics = i;
        }
        
        // 计算队列
        if (!indices.compute.has_value() && (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
            indices.compute = i;
        }
        
        // 传输队列
        if (!indices.transfer.has_value() && (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)) {
            indices.transfer = i;
        }
        
        // 呈现队列
        if (presentSurface != nullptr && !indices.present.has_value()) {
            const VkBool32 presentSupport = m_device.getSurfaceSupportKHR(i, *presentSurface->GetHandle());
            if (presentSupport == VK_TRUE) {
                indices.present = i;
            }
        }
    }
    
    return indices;
}

// ============================================================================
// 设备评分
// ============================================================================

uint32_t VulkanPhysicalDevice::CalculateScore() const {
    const auto properties = GetProperties();
    const auto features = GetFeatures();
    
    uint32_t score = 0;
    
    // 设备类型评分
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 1000;
    } else if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
        score += 500;
    }
    
    // 纹理尺寸
    score += properties.limits.maxImageDimension2D;
    
    // 特性加分
    if (features.geometryShader) score += 100;
    if (features.tessellationShader) score += 100;
    if (features.samplerAnisotropy) score += 50;
    
    return score;
}

// ============================================================================
// 扩展检查
// ============================================================================

bool VulkanPhysicalDevice::CheckExtensionSupport(const std::vector<const char*>& extensions) const {
    return CheckDeviceExtensionSupport(*m_device, extensions);
}

// ============================================================================
// 调试信息
// ============================================================================

void VulkanPhysicalDevice::PrintInfo() const {
    const auto properties = GetProperties();
    const auto features = GetFeatures();
    const auto memProperties = GetMemoryProperties();
    const auto queueFamilies = GetQueueFamilies();
    
    TE_LOG_INFO("========== Physical Device ==========");
    TE_LOG_INFO("Name: {}", properties.deviceName.data());
    TE_LOG_INFO("Type: {}", vk::to_string(properties.deviceType));
    TE_LOG_INFO("API: {}.{}.{}", 
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));
    TE_LOG_INFO("Driver: 0x{:X}", properties.driverVersion);
    
    // 内存
    TE_LOG_INFO("Memory Heaps: {}", memProperties.memoryHeapCount);
    for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i) {
        const auto& heap = memProperties.memoryHeaps[i];
        TE_LOG_INFO("  [{}] {} MB", i, heap.size / (1024 * 1024));
    }
    
    // 队列族
    TE_LOG_INFO("Queue Families: {}", queueFamilies.size());
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        const auto& family = queueFamilies[i];
        TE_LOG_INFO("  [{}] Count={}, Flags={}", 
                    i, family.queueCount,
                    vk::to_string(vk::QueueFlags(family.queueFlags)));
    }
    
    // 关键特性
    TE_LOG_INFO("Features:");
    TE_LOG_INFO("  Geometry Shader: {}", features.geometryShader ? "Yes" : "No");
    TE_LOG_INFO("  Tessellation: {}", features.tessellationShader ? "Yes" : "No");
    TE_LOG_INFO("  Anisotropy: {}", features.samplerAnisotropy ? "Yes" : "No");
    
    TE_LOG_INFO("Score: {}", CalculateScore());
    TE_LOG_INFO("====================================");
}

} // namespace TE

