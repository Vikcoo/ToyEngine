/*
文件用途: 交换链封装
- 负责: 查询 Surface 能力/格式/呈现模式，创建 Swapchain，并获取可呈现图像
- 概念: Swapchain 管理多个呈现图像(BackBuffers)，Acquire/Present 驱动帧循环
*/
#pragma once
#include <memory>
#include "TEVKCommon.h"

namespace TE {
    class TEVKGraphicContext;
    class TEVKLogicDevice;

    // Surface 支持信息（当前实现选取一个最合适的格式/模式）
    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::PresentModeKHR presentMode;
    };
    class TEVKSwapChain {
        public:
        TEVKSwapChain(TEVKGraphicContext& context, TEVKLogicDevice& logicDevice);
        // 重建交换链（通常在窗口尺寸变化或格式变更时）
        bool ReCreate();
        // 查询 Surface 支持并选择合适的 format/presentMode 等
        SwapChainSupportDetails QuerySwapChainSupport();
        const std::vector<vk::Image>& GetImages() const { return m_images; }
        uint32_t GetWidth() const {return m_surfaceInfo.capabilities.currentExtent.width; }
        uint32_t GetHeight() const {return m_surfaceInfo.capabilities.currentExtent.height; }
        const vk::raii::SwapchainKHR& GetHandle() const {return m_handle;}
        // 获取下一张可用图像（支持信号量/栅栏同步）
        uint32_t AcquireImage(vk::Semaphore semaphore, vk::Fence fence = VK_NULL_HANDLE) const;
        // 呈现图像到屏幕
        void Present(uint32_t imageIndex, const std::vector<vk::Semaphore>& waitSemaphores) const;
        private:
        TEVKGraphicContext& m_context;
        TEVKLogicDevice& m_logicDevice;
        SwapChainSupportDetails m_surfaceInfo;

        vk::raii::SwapchainKHR m_handle{nullptr};

        std::vector<vk::Image> m_images;
    };
}