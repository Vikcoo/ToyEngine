// Vulkan Physical Device - 物理设备查询和选择
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace TE {

class VulkanContext;
class VulkanSurface;

/// 队列族索引
struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
    
    [[nodiscard]] bool IsComplete() const {
        return graphics.has_value() && present.has_value();
    }
    
    [[nodiscard]] bool IsSameGraphicsPresent() const {
        return graphics.has_value() && present.has_value() && 
               graphics.value() == present.value();
    }
};

/// Vulkan 物理设备 - 查询硬件信息和能力
class VulkanPhysicalDevice {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanContext;
    };

    VulkanPhysicalDevice(PrivateTag, std::weak_ptr<VulkanContext> context, vk::raii::PhysicalDevice device)
    : m_context(std::move(context)), m_device(std::move(device)) {}

    // 允许移动，禁用拷贝
    VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice(VulkanPhysicalDevice&&) noexcept = default;
    VulkanPhysicalDevice& operator=(VulkanPhysicalDevice&&) noexcept = default;

    // 属性查询
    [[nodiscard]] vk::PhysicalDeviceProperties GetProperties() const;
    [[nodiscard]] vk::PhysicalDeviceFeatures GetFeatures() const;
    [[nodiscard]] vk::PhysicalDeviceMemoryProperties GetMemoryProperties() const;
    [[nodiscard]] std::vector<vk::QueueFamilyProperties> GetQueueFamilies() const;
    [[nodiscard]] std::string GetDeviceName() const;

    // 队列族查找
    [[nodiscard]] std::optional<uint32_t> FindQueueFamily(
        vk::QueueFlags queueFlags,
        const VulkanSurface* presentSurface = nullptr
    ) const;
    
    [[nodiscard]] QueueFamilyIndices FindQueueFamilies(
        const VulkanSurface* presentSurface = nullptr
    ) const;

    // 设备评分（用于选择最佳设备）
    [[nodiscard]] uint32_t CalculateScore() const;

    // 扩展检查
    [[nodiscard]] bool CheckExtensionSupport(const std::vector<const char*>& extensions) const;

    // 查询符合要求的内存类型索引
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    // 查询深度图像格式
    vk::Format FindDepthFormat( const std::vector<vk::Format>& candidates ) const;

    // 查询采样数
    vk::SampleCountFlagBits GetMaxUsableSampleCount() const;

    // 获取底层句柄
    [[nodiscard]] const vk::raii::PhysicalDevice& GetHandle() const { return m_device; }

    // 调试信息
    void PrintInfo() const;
private:
    std::weak_ptr<VulkanContext> m_context;
    vk::raii::PhysicalDevice m_device;

    // 查询结果缓存
    mutable std::optional<vk::PhysicalDeviceProperties> m_cachedProperties;
    mutable std::optional<vk::PhysicalDeviceFeatures> m_cachedFeatures;
    mutable std::optional<std::vector<vk::QueueFamilyProperties>> m_cachedQueueFamilies;
};

} // namespace TE

