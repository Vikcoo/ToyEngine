//
// Created by yukai on 2025/8/10.
//
#pragma once

#include <memory>

#include "vulkan/vulkan.hpp"


namespace TE {
    class TEVKGraphicContext;
    class TEVKQueue;

    struct TEVKSetting {

    };
    class TEVKLogicDevice {
        public:
        TEVKLogicDevice(TEVKGraphicContext* context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const TEVKSetting& setting = {});
        ~TEVKLogicDevice();
        private:
        vk::Device m_logicDevice;
        std::vector<std::shared_ptr<TEVKQueue>> m_graphicQueue;
        std::vector<std::shared_ptr<TEVKQueue>> m_presentQueue;
    };
}
