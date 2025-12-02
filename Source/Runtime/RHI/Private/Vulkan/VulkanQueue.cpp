// Vulkan Queue 实现
#include "VulkanQueue.h"
#include "Log/Log.h"

namespace TE {

VulkanQueue::VulkanQueue(PrivateTag,
                         vk::raii::Queue queue,
                         const uint32_t familyIndex,
                         const uint32_t queueIndex)
    : m_queue(std::move(queue))
    , m_familyIndex(familyIndex)
    , m_queueIndex(queueIndex)
{
    TE_LOG_DEBUG("Queue created: Family={}, Index={}", familyIndex, queueIndex);
}

void VulkanQueue::Submit(
    const std::vector<vk::CommandBuffer>& cmdBuffers,
    const std::vector<vk::Semaphore>& waitSemaphores,
    const std::vector<vk::PipelineStageFlags>& waitStages,
    const std::vector<vk::Semaphore>& signalSemaphores,
    const vk::Fence fence)
{
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(cmdBuffers)
              .setWaitSemaphores(waitSemaphores)
              .setWaitDstStageMask(waitStages)
              .setSignalSemaphores(signalSemaphores);

    try {
        m_queue.submit(submitInfo, fence);
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Queue submit failed: {}", e.what());
        throw;
    }
}

void VulkanQueue::WaitIdle() {
    try {
        m_queue.waitIdle();
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Queue waitIdle failed: {}", e.what());
        throw;
    }
}

} // namespace TE


