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
    class TESwapChain {
        public:
        TESwapChain(TEVKGraphicContext* context, TEVKLogicDevice* logicDevice);
        ~TESwapChain();
        bool ReCreate();
        void SetupSurfaceCapabilities();
        private:
        vk::SwapchainKHR m_handle = VK_NULL_HANDLE;
        TEVKGraphicContext* m_context;
        TEVKLogicDevice* m_logicDevice;

        vk::SurfaceCapabilitiesKHR m_surfaceCapabilities;
    };
}
