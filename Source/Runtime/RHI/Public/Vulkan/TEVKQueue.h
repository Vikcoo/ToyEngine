//
// Created by yukai on 2025/8/10.
//
#pragma once

#include "TEVKCommon.h"

namespace TE {
    class TEVKQueue {
        public:
        TEVKQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, vk::raii::Queue queue, bool isPresent);
        void WaitIdle() const;
        const vk::raii::Queue& GetHandle(){return m_handle;}
        void Submit(std::vector<vk::CommandBuffer>& commandBuffers);
        private:
        uint32_t m_queueFamilyIndex;
        uint32_t m_queueIndex;
        vk::raii::Queue m_handle;
        bool m_isPresent;
    };

}
