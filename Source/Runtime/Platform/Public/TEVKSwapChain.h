//
// Created by yukai on 2025/8/12.
//
//
// Created by yukai on 2025/8/10.
//
#pragma once

#include <memory>

#include "TEVKCommon.h"


namespace TE {
    class TEVKGraphicContext;
    class TEVKLogicDevice;

    struct SurfaceInfo {
        vk::SurfaceCapabilitiesKHR capabilities;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::PresentModeKHR presentMode;
    };
    class TEVKSwapChain {
        public:
        TEVKSwapChain(TEVKGraphicContext* context, TEVKLogicDevice* logicDevice);
        ~TEVKSwapChain();
        bool ReCreate();
        void SetupSurfaceCapabilities();
        const std::vector<vk::Image> GetImages() const { return m_images; }
        uint32_t GetWidth() const {return m_surfaceInfo.capabilities.currentExtent.width; }
        uint32_t GetHeight() const {return m_surfaceInfo.capabilities.currentExtent.height; }
        private:
        vk::SwapchainKHR m_handle = VK_NULL_HANDLE;
        TEVKGraphicContext* m_context;
        TEVKLogicDevice* m_logicDevice;

        SurfaceInfo m_surfaceInfo;

        std::vector<vk::Image> m_images;
    };
}
