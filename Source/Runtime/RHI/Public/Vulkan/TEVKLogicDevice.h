//
// Created by yukai on 2025/8/10.
//
#pragma once

#include <memory>
#include "TEVKCommon.h"


namespace TE {
    class TEVKCommandPool;
    class TEVKGraphicContext;
    class TEVKQueue;

    struct TEVKSetting {
        vk::Format format = vk::Format::eR8G8B8A8Unorm;
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;
        uint32_t swapChainImageCount = 3;
    };

    class TEVKLogicDevice {
        public:
        TEVKLogicDevice(TEVKGraphicContext& context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const TEVKSetting& setting = {});
        vk::raii::Device* GetHandle(){return &m_handle;}
        const TEVKSetting& GetSetting() const {return m_setting;}
        const vk::raii::PipelineCache& GetPipelineCache(){return m_pipelineCache;}

        [[nodiscard]] TEVKQueue* GetGraphicQueue(uint32_t index) const { return m_graphicQueue.size() < index + 1 ? nullptr : m_graphicQueue[index].get(); };
        [[nodiscard]] TEVKQueue* GetFirstGraphicQueue() const { return m_graphicQueue.empty() ? nullptr : m_graphicQueue[0].get(); };
        [[nodiscard]] TEVKQueue* GetPresentQueue(uint32_t index) const { return m_presentQueue.size() < index + 1 ? nullptr : m_presentQueue[index].get(); };
        [[nodiscard]] TEVKQueue* GetFirstPresentQueue() const { return m_presentQueue.empty() ? nullptr : m_presentQueue[0].get(); };

        private:
        void CreatePipelineCache();

        vk::raii::Device m_handle{nullptr};
        std::vector<std::shared_ptr<TEVKQueue>> m_graphicQueue;
        std::vector<std::shared_ptr<TEVKQueue>> m_presentQueue;
        vk::raii::PipelineCache m_pipelineCache{nullptr};

        TEVKSetting m_setting;
    };
}
