/*
文件用途: 队列封装
- 负责: 保存队列族索引与队列索引，提供 WaitIdle/Submit 等基础能力
- 概念: 命令缓冲通过 Submit 提交到队列，VkSemaphore/VkFence 用于同步
*/
#pragma once

#include "TEVKCommon.h"

namespace TE {
    class TEVKQueue {
        public:
        TEVKQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, vk::raii::Queue queue, bool isPresent);
        // 等待队列空闲（通常用于资源重建/退出）
        void WaitIdle() const;
        const vk::raii::Queue& GetHandle(){return m_handle;}
        // 提交命令到队列，可指定等待/信号信号量与栅栏
        void Submit(std::vector<vk::CommandBuffer>& commandBuffers, const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores, vk::Fence fence);
        private:
        uint32_t m_queueFamilyIndex;
        uint32_t m_queueIndex;
        vk::raii::Queue m_handle;
        bool m_isPresent;
    };

}