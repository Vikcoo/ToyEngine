//
// Created by yukai on 2025/8/12.
//
#include "TEVKSwapChain.h"

#include "TEVKGraphicContext.h"
#include "TEVKLogicDevice.h"

namespace TE {
    TEVKSwapChain::TEVKSwapChain(TEVKGraphicContext *context, TEVKLogicDevice *logicDevice): m_context(context), m_logicDevice(logicDevice) {
        ReCreate();

    }

    TEVKSwapChain::~TEVKSwapChain() {
        m_logicDevice->GetHandle()->destroySwapchainKHR(m_handle);
    }

    bool TEVKSwapChain::ReCreate() {
        //todo：销毁旧的chain
        SetupSurfaceCapabilities();


        vk::SwapchainCreateInfoKHR swapChainCreateInfo;
        uint32_t imageCount = m_logicDevice->GetSetting().swapChainImageCount;
        if (imageCount < m_surfaceInfo.capabilities.minImageCount && m_surfaceInfo.capabilities.minImageCount > 0) {
            imageCount = m_surfaceInfo.capabilities.minImageCount;
        }
        if (imageCount > m_surfaceInfo.capabilities.maxImageCount && m_surfaceInfo.capabilities.maxImageCount > 0) {
            imageCount = m_surfaceInfo.capabilities.maxImageCount;
        }

        swapChainCreateInfo.setMinImageCount(imageCount);
        swapChainCreateInfo.setImageColorSpace(m_surfaceInfo.surfaceFormat.colorSpace);
        swapChainCreateInfo.setSurface(m_context->GetSurface());
        swapChainCreateInfo.setImageFormat(m_surfaceInfo.surfaceFormat.format);
        swapChainCreateInfo.setImageExtent(m_surfaceInfo.capabilities.currentExtent);
        swapChainCreateInfo.setImageArrayLayers(1);
        swapChainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

        vk::SharingMode sharingMode;
        uint32_t queueFamilyIndexCount = 0;
        uint32_t pQueueFamilyIndices[2] = { 0, 0 };
        if (m_context->IsSameGraphicPresentQueueFamily()) {
            sharingMode = vk::SharingMode::eExclusive;
            queueFamilyIndexCount = 0;
        }else {
            sharingMode = vk::SharingMode::eConcurrent;
            queueFamilyIndexCount = 2;
            pQueueFamilyIndices[0] = m_context->GetGraphicQueueFamilyInfo().queueFamilyIndex;
            pQueueFamilyIndices[1] = m_context->GetPresentQueueFamilyInfo().queueFamilyIndex;
        }

        vk::SwapchainKHR oldSwapChain = m_handle;
        swapChainCreateInfo.setImageSharingMode(sharingMode);
        swapChainCreateInfo.setQueueFamilyIndexCount(queueFamilyIndexCount);
        swapChainCreateInfo.setPQueueFamilyIndices(pQueueFamilyIndices);
        swapChainCreateInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapChainCreateInfo.setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eInherit);
        swapChainCreateInfo.setPresentMode(m_surfaceInfo.presentMode);
        swapChainCreateInfo.setClipped(false);
        swapChainCreateInfo.setOldSwapchain(oldSwapChain);

        CALL_VK_CHECK(m_logicDevice->GetHandle()->createSwapchainKHR(&swapChainCreateInfo, nullptr, &m_handle));

        LOG_TRACE("Swap chain created, func: {}, old: {}, new: {}, imageCount: {}, format: {}, presentMode: {}",__FUNCTION__,(void*) oldSwapChain, (void*)m_handle, imageCount,vk::to_string(m_surfaceInfo.surfaceFormat.format), vk::to_string(m_surfaceInfo.presentMode));

        m_images = m_logicDevice->GetHandle()->getSwapchainImagesKHR(m_handle);
        return !m_images.empty();
    }

    void TEVKSwapChain::SetupSurfaceCapabilities() {
        // capabilities
        m_surfaceInfo.capabilities = m_context->GetPhysicalDevice().getSurfaceCapabilitiesKHR(m_context->GetSurface());

        // Format
        std::vector<vk::SurfaceFormatKHR> surfaceFormats = m_context->GetPhysicalDevice().getSurfaceFormatsKHR(m_context->GetSurface());;
        LOG_TRACE("getSurfaceFormatsKHR NUM:{}",surfaceFormats.size());
        TEVKSetting setting = m_logicDevice->GetSetting();
        uint32_t foundFormatCount = -1;
        for (uint32_t i = 0; i < surfaceFormats.size(); i++) {
            if (surfaceFormats[i].format == setting.format && surfaceFormats[i].colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
                foundFormatCount = i;
                break;
            }
        }
        if (foundFormatCount == -1) {
            foundFormatCount = 0;
            LOG_ERROR("Could not find a suitable surface format.");
        }
        m_surfaceInfo.surfaceFormat = surfaceFormats[foundFormatCount];


        // Format
        std::vector<vk::PresentModeKHR> presentModeKHRs = m_context->GetPhysicalDevice().getSurfacePresentModesKHR(m_context->GetSurface());;
        LOG_TRACE("getSurfacePresentModesKHR NUM:{}",surfaceFormats.size());
        vk::PresentModeKHR presentMode = setting.presentMode;
        uint32_t foundPresentModeCount = -1;
        for (uint32_t i = 0; i < presentModeKHRs.size(); i++) {
            if (presentModeKHRs[i] == presentMode) {
                foundPresentModeCount = i;
                break;
            }
        }

        if (foundPresentModeCount == -1) {
            foundFormatCount = 0;
            LOG_ERROR("Could not find a suitable surface Present Mode.");
        }
        m_surfaceInfo.presentMode = presentModeKHRs[foundPresentModeCount];
    }
}
