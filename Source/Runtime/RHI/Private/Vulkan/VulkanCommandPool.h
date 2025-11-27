// Vulkan Command Pool - 命令池
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace TE {

class VulkanDevice;
class VulkanCommandBuffer;

/// Vulkan 命令池 - 分配和管理命令缓冲
class VulkanCommandPool {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    explicit VulkanCommandPool(PrivateTag,
                              std::shared_ptr<VulkanDevice> device,
                              uint32_t queueFamilyIndex,
                              vk::CommandPoolCreateFlags flags = {});

    ~VulkanCommandPool();

    // 禁用拷贝，允许移动
    VulkanCommandPool(const VulkanCommandPool&) = delete;
    VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
    VulkanCommandPool(VulkanCommandPool&&) noexcept = default;
    VulkanCommandPool& operator=(VulkanCommandPool&&) noexcept = default;

    // 分配命令缓冲
    [[nodiscard]] std::vector<vk::raii::CommandBuffer> AllocateBuffers(
        uint32_t count,
        vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
    );

    // 重置命令池
    void Reset(vk::CommandPoolResetFlags flags = {});

    // 获取信息
    [[nodiscard]] uint32_t GetQueueFamilyIndex() const { return m_queueFamilyIndex; }
    [[nodiscard]] const vk::raii::CommandPool& GetHandle() const { return m_pool; }

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::CommandPool m_pool{nullptr};
    uint32_t m_queueFamilyIndex;
};

} // namespace TE


