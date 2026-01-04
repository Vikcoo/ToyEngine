// Vulkan Descriptor Pool - 描述符池
#pragma once

#include "../Core/VulkanDevice.h"
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <memory>

namespace TE {

class VulkanDevice;

/// 描述符池大小配置
struct DescriptorPoolSize {
    vk::DescriptorType type;     // 描述符类型
    uint32_t count;              // 该类型的描述符数量
};

/// Vulkan 描述符池
class VulkanDescriptorPool {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    explicit VulkanDescriptorPool(
        PrivateTag,
        std::shared_ptr<VulkanDevice> device,
        uint32_t maxSets,                              // 最大描述符集数量
        const std::vector<DescriptorPoolSize>& poolSizes, // 各类型描述符数量
        vk::DescriptorPoolCreateFlags flags = {}        // 创建标志
    );

    ~VulkanDescriptorPool();

    // 禁用拷贝，允许移动
    VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool(VulkanDescriptorPool&&) noexcept = default;
    VulkanDescriptorPool& operator=(VulkanDescriptorPool&&) noexcept = default;

    // 分配描述符集
    [[nodiscard]] std::vector<vk::raii::DescriptorSet> AllocateDescriptorSets(
        const std::vector<vk::DescriptorSetLayout>& layouts
    );

    // 获取句柄
    [[nodiscard]] const vk::raii::DescriptorPool& GetHandle() const { return m_pool; }

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::DescriptorPool m_pool{nullptr};
};

} // namespace TE

