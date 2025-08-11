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
        vk::Format format = vk::Format::eR8G8B8A8Unorm;
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;
        uint32_t swapChainImageCount = 3;
    };
    class TEVKLogicDevice {
        public:
        TEVKLogicDevice(TEVKGraphicContext* context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const TEVKSetting& setting = {});
        ~TEVKLogicDevice();
        vk::Device* GetHandle(){return &m_handle;}
        private:
        vk::Device m_handle;
        std::vector<std::shared_ptr<TEVKQueue>> m_graphicQueue;
        std::vector<std::shared_ptr<TEVKQueue>> m_presentQueue;
        TEVKSetting m_setting;
    };
}
