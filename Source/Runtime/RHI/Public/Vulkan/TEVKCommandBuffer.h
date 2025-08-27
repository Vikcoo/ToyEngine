//
// Created by yukai on 2025/8/24.
//
#pragma once
#include "TEVKCommon.h"

namespace TE{
    class TEVKLogicDevice;

    class TEVKCommandPool{
    public:
        TEVKCommandPool(TEVKLogicDevice &device, uint32_t queueFamilyIndex);

        static void BeginCommandBuffer(vk::raii::CommandBuffer& cmdBuffer);
        static void EndCommandBuffer(vk::raii::CommandBuffer& cmdBuffer);

        [[nodiscard]] std::vector<vk::raii::CommandBuffer> AllocateCommandBuffer(uint32_t count) const;
        [[nodiscard]] const vk::raii::CommandPool& GetHandle() const { return m_handle; }
    private:
        vk::raii::CommandPool m_handle{ nullptr };

        TEVKLogicDevice &m_device;
    };
}
