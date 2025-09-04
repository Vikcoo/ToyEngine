//
// Created by yukai on 2025/8/10.
//
#include "Vulkan/TEVKQueue.h"
namespace TE {
    TEVKQueue::TEVKQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, vk::raii::Queue queue, bool isPresent)
        :m_queueFamilyIndex(queueFamilyIndex),m_queueIndex(queueIndex),m_handle(queue),m_isPresent(isPresent){
        LOG_TRACE(" Creating a new queue FamilyIndex:{}--queueIndex:{}--isPresent:{}",m_queueFamilyIndex,m_queueIndex,m_isPresent);
    }

    void TEVKQueue::WaitIdle() const {
        m_handle.waitIdle();
    }

    void TEVKQueue::Submit(std::vector<vk::CommandBuffer>& commandBuffers, const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence) {
        std::vector<vk::PipelineStageFlags>  waitDstStageMask = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        vk::SubmitInfo submitInfo;
        submitInfo.setWaitSemaphores(waitSemaphores);
        submitInfo.pWaitDstStageMask = waitDstStageMask.data();
        submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.setSignalSemaphores(signalSemaphores);
        m_handle.submit(submitInfo,fence);
    }
}
