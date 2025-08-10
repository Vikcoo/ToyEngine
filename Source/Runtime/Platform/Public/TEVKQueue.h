//
// Created by yukai on 2025/8/10.
//
#pragma once
#include "TEVKCommon.h"



namespace TE {
    class TEVKQueue {
        public:
        TEVKQueue(uint32_t queueFamilyIndex, uint32_t queueIndex, vk::Queue queue, bool isPresent);
        ~TEVKQueue() = default;
        void WaitIdle() const;
        private:
        uint32_t m_queueFamilyIndex;
        uint32_t m_queueIndex;
        vk::Queue m_queue;
        bool m_isPresent;
    };

}
