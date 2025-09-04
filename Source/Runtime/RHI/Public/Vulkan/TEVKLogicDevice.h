/*
文件用途: Vulkan 逻辑设备与队列封装
- 负责: 基于物理设备与队列族信息创建 vk::Device，获取图形/呈现队列，创建 PipelineCache
- 概念: 逻辑设备是对物理设备的抽象实例，队列是提交命令的执行载体
*/
#pragma once

#include <memory>
#include "TEVKCommon.h"

namespace TE {
    class TEVKCommandPool;
    class TEVKGraphicContext;
    class TEVKQueue;

    // 运行时图形设置：用于驱动交换链/渲染通道等配置
    struct TEVKSetting {
        vk::Format format = vk::Format::eR8G8B8A8Unorm;           // 目标像素格式（通常与表面格式匹配）
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate; // 呈现模式（如 FIFO/MAILBOX/IMMEDIATE）
        uint32_t swapChainImageCount = 3;                         // 交换链图像数量
    };

    class TEVKLogicDevice {
        public:
        TEVKLogicDevice(TEVKGraphicContext& context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const TEVKSetting& setting = {});
        vk::raii::Device* GetHandle(){return &m_handle;}
        const TEVKSetting& GetSetting() const {return m_setting;}
        const vk::raii::PipelineCache& GetPipelineCache(){return m_pipelineCache;}

        // 获取图形/呈现队列（简化常用获取方式）
        [[nodiscard]] TEVKQueue* GetGraphicQueue(uint32_t index) const { return m_graphicQueue.size() < index + 1 ? nullptr : m_graphicQueue[index].get(); };
        [[nodiscard]] TEVKQueue* GetFirstGraphicQueue() const { return m_graphicQueue.empty() ? nullptr : m_graphicQueue[0].get(); };
        [[nodiscard]] TEVKQueue* GetPresentQueue(uint32_t index) const { return m_presentQueue.size() < index + 1 ? nullptr : m_presentQueue[index].get(); };
        [[nodiscard]] TEVKQueue* GetFirstPresentQueue() const { return m_presentQueue.empty() ? nullptr : m_presentQueue[0].get(); };

        private:
        // 创建 PipelineCache（用于复用/加速管线创建）
        void CreatePipelineCache();

        vk::raii::Device m_handle{nullptr};
        std::vector<std::shared_ptr<TEVKQueue>> m_graphicQueue;
        std::vector<std::shared_ptr<TEVKQueue>> m_presentQueue;
        vk::raii::PipelineCache m_pipelineCache{nullptr};

        TEVKSetting m_setting;
    };
}