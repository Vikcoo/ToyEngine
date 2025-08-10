//
// Created by yukai on 2025/8/10.
//
#include "TEVKQueue.h"
namespace TE {
    TEVKQueue::TEVKQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, vk::Queue queue, bool isPresent)
        :m_queueFamilyIndex(queueFamilyIndex),m_queueIndex(queueIndex),m_queue(queue),m_isPresent(isPresent){
        LOG_TRACE(" Creating a new queue {}--{}--{}",m_queueFamilyIndex,m_queueIndex,m_isPresent);
    }

    void TEVKQueue::WaitIdle() const {
        m_queue.waitIdle();
    }
}
