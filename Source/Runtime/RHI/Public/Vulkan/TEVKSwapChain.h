//
// Created by yukai on 2025/8/12.
//
#pragma once
#include <memory>
#include "TEVKCommon.h"

namespace TE {
    class TEVKGraphicContext;
    class TEVKLogicDevice;

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::PresentModeKHR presentMode;
    };
    class TEVKSwapChain {
        public:
        TEVKSwapChain(TEVKGraphicContext& context, TEVKLogicDevice& logicDevice);
        bool ReCreate();
        SwapChainSupportDetails QuerySwapChainSupport();
        const std::vector<vk::Image>& GetImages() const { return m_images; }
        uint32_t GetWidth() const {return m_surfaceInfo.capabilities.currentExtent.width; }
        uint32_t GetHeight() const {return m_surfaceInfo.capabilities.currentExtent.height; }
        const vk::raii::SwapchainKHR& GetHandle() const {return m_handle;}
        uint32_t AcquireImage() const;
        void Present(uint32_t imageIndex) const;
        private:
        TEVKGraphicContext& m_context;
        TEVKLogicDevice& m_logicDevice;
        SwapChainSupportDetails m_surfaceInfo;

        vk::raii::SwapchainKHR m_handle{nullptr};

        std::vector<vk::Image> m_images;
    };
}
