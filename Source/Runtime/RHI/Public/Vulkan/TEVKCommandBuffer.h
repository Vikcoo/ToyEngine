/*
文件用途: 命令池与命令缓冲封装
- 负责: 创建 CommandPool、分配 CommandBuffer，提供简化的 Begin/End 录制辅助
- 概念: 命令缓冲承载绘制/资源操作命令；需从与队列族匹配的命令池中分配
*/
#pragma once
#include "TEVKCommon.h"

namespace TE{
    class TEVKLogicDevice;

    // 命令池封装：从指定队列族索引创建，分配/管理 CommandBuffer
    class TEVKCommandPool{
    public:
        TEVKCommandPool(TEVKLogicDevice &device, uint32_t queueFamilyIndex);

        // 录制辅助：开始/结束命令缓冲的录制
        static void BeginCommandBuffer(vk::raii::CommandBuffer& cmdBuffer);
        static void EndCommandBuffer(vk::raii::CommandBuffer& cmdBuffer);

        // 分配多个 Primary 级别的命令缓冲
        [[nodiscard]] std::vector<vk::raii::CommandBuffer> AllocateCommandBuffer(uint32_t count) const;
        [[nodiscard]] const vk::raii::CommandPool& GetHandle() const { return m_handle; }
    private:
        vk::raii::CommandPool m_handle{ nullptr };

        TEVKLogicDevice &m_device;
    };
}