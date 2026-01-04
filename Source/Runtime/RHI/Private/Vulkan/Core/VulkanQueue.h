// Vulkan Queue - 队列封装
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>

namespace TE {

/// Vulkan 队列 - 用于提交命令和同步
class VulkanQueue {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    explicit VulkanQueue(PrivateTag,
                        vk::raii::Queue queue,
                        uint32_t familyIndex,
                        uint32_t queueIndex);

    // 禁用拷贝和移动
    VulkanQueue(const VulkanQueue&) = delete;
    VulkanQueue& operator=(const VulkanQueue&) = delete;
    VulkanQueue(VulkanQueue&&) = delete;
    VulkanQueue& operator=(VulkanQueue&&) = delete;

    // 提交命令
    void Submit(
        const std::vector<vk::CommandBuffer>& cmdBuffers,
        const std::vector<vk::Semaphore>& waitSemaphores = {},
        const std::vector<vk::PipelineStageFlags>& waitStages = {},
        const std::vector<vk::Semaphore>& signalSemaphores = {},
        vk::Fence fence = nullptr
    );

    // 等待队列空闲
    void WaitIdle();

    // 获取信息
    [[nodiscard]] uint32_t GetFamilyIndex() const { return m_familyIndex; }
    [[nodiscard]] uint32_t GetQueueIndex() const { return m_queueIndex; }
    [[nodiscard]] const vk::raii::Queue& GetHandle() const { return m_queue; }

private:
    vk::raii::Queue m_queue;
    uint32_t m_familyIndex;
    uint32_t m_queueIndex;
};

} // namespace TE


