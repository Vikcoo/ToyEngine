//
// Created by yukai on 2025/8/12.
//
#include "TESwapChain.h"

#include "TEVKGraphicContext.h"
#include "TEVKLogicDevice.h"

namespace TE {
    TESwapChain::TESwapChain(TEVKGraphicContext *context, TEVKLogicDevice *logicDevice): m_context(context), m_logicDevice(logicDevice) {
        ReCreate();

    }

    TESwapChain::~TESwapChain() {
    }

    bool TESwapChain::ReCreate() {



        vk::SwapchainCreateInfoKHR swapChainCreateInfo;

        CALL_VK_CHECK(m_logicDevice->GetHandle()->createSwapchainKHR(&swapChainCreateInfo, nullptr, &m_handle));
    }

    void TESwapChain::SetupSurfaceCapabilities() {
        m_surfaceCapabilities = m_context->GetPhysicalDevice().getSurfaceCapabilitiesKHR(m_context->GetSurface());
    }
}
