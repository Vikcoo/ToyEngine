//
// Created by yukai on 2025/8/12.
//
#include "Vulkan/TEVKSwapChain.h"

#include "Vulkan/TEVKGraphicContext.h"
#include "Vulkan/TEVKLogicDevice.h"
#include "Vulkan/TEVKQueue.h"

namespace TE {
    TEVKSwapChain::TEVKSwapChain(TEVKGraphicContext &context, TEVKLogicDevice &logicDevice): m_context(context), m_logicDevice(logicDevice) {
        ReCreate();
    }

    bool TEVKSwapChain::ReCreate() {
        //todo：销毁旧的chain
        m_surfaceInfo = QuerySwapChainSupport();

        uint32_t imageCount = m_logicDevice.GetSetting().swapChainImageCount;
        if (imageCount < m_surfaceInfo.capabilities.minImageCount && m_surfaceInfo.capabilities.minImageCount > 0) {
            imageCount = m_surfaceInfo.capabilities.minImageCount;
        }
        if (imageCount > m_surfaceInfo.capabilities.maxImageCount && m_surfaceInfo.capabilities.maxImageCount > 0) {
            imageCount = m_surfaceInfo.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR swapChainCreateInfo;
        swapChainCreateInfo.setMinImageCount(imageCount);
        swapChainCreateInfo.setImageColorSpace(m_surfaceInfo.surfaceFormat.colorSpace);
        swapChainCreateInfo.setSurface(m_context.GetSurface());
        swapChainCreateInfo.setImageFormat(m_surfaceInfo.surfaceFormat.format);
        swapChainCreateInfo.setImageExtent(m_surfaceInfo.capabilities.currentExtent);
        swapChainCreateInfo.setImageArrayLayers(1);
        swapChainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

        vk::SharingMode sharingMode;
        uint32_t queueFamilyIndexCount = 0;
        uint32_t pQueueFamilyIndices[2] = { 0, 0 };
        if (m_context.IsSameGraphicPresentQueueFamily()) {
            sharingMode = vk::SharingMode::eExclusive;
            queueFamilyIndexCount = 0;
        }else {
            sharingMode = vk::SharingMode::eConcurrent;
            queueFamilyIndexCount = 2;
            pQueueFamilyIndices[0] = m_context.GetGraphicQueueFamilyInfo().queueFamilyIndex;
            pQueueFamilyIndices[1] = m_context.GetPresentQueueFamilyInfo().queueFamilyIndex;
        }

        vk::SwapchainKHR oldSwapChain = m_handle;
        swapChainCreateInfo.setImageSharingMode(sharingMode);
        swapChainCreateInfo.setQueueFamilyIndexCount(queueFamilyIndexCount);
        swapChainCreateInfo.setPQueueFamilyIndices(pQueueFamilyIndices);
        swapChainCreateInfo.setPreTransform(m_surfaceInfo.capabilities.currentTransform);
        swapChainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapChainCreateInfo.setPresentMode(m_surfaceInfo.presentMode);
        swapChainCreateInfo.setClipped(true);
        swapChainCreateInfo.setOldSwapchain(oldSwapChain);

        m_handle = m_logicDevice.GetHandle()->createSwapchainKHR(swapChainCreateInfo);
        if (m_handle == VK_NULL_HANDLE) {
            LOG_ERROR("Swap chain created Failed");
            throw std::runtime_error("Swap chain creation failed");
        }
        LOG_TRACE("Swap chain created, func: {}, old: {}, new: {}, imageCount: {}, format: {}, presentMode: {}",__FUNCTION__,(void*) oldSwapChain, reinterpret_cast<uint64_t>(static_cast<void*>(*m_handle)), imageCount,vk::to_string(m_surfaceInfo.surfaceFormat.format), vk::to_string(m_surfaceInfo.presentMode));
        m_images = m_handle.getImages();
        return !m_images.empty();
    }

    SwapChainSupportDetails TEVKSwapChain::QuerySwapChainSupport() {
        SwapChainSupportDetails details;
        details.capabilities = m_context.GetPhysicalDevice().getSurfaceCapabilitiesKHR(m_context.GetSurface());
        std::vector<vk::SurfaceFormatKHR> surfaceFormats = m_context.GetPhysicalDevice().getSurfaceFormatsKHR(m_context.GetSurface());
        LOG_TRACE("getSurfaceFormatsKHR NUM:{}",surfaceFormats.size());
        TEVKSetting setting = m_logicDevice.GetSetting();
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
        details.surfaceFormat = surfaceFormats[foundFormatCount];

        // presentMode
        std::vector<vk::PresentModeKHR> presentModeKHRs = m_context.GetPhysicalDevice().getSurfacePresentModesKHR(m_context.GetSurface());;
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
        details.presentMode = presentModeKHRs[foundPresentModeCount];

        return details;
    }

    uint32_t TEVKSwapChain::AcquireImage(vk::Semaphore semaphore, vk::Fence fence) const {
        auto [result, imageIndex] = m_handle.acquireNextImage(UINT64_MAX,semaphore,fence);
        if (fence != VK_NULL_HANDLE) {
            CALL_VK_CHECK(m_logicDevice.GetHandle()->waitForFences(fence, 1, UINT64_MAX));
            m_logicDevice.GetHandle()->resetFences(fence);
        }
        return imageIndex;
    }

    void TEVKSwapChain::Present(uint32_t imageIndex, const std::vector<vk::Semaphore>& waitSemaphores) const {
        vk::PresentInfoKHR presentInfo;
        presentInfo.setSwapchains(*m_handle);
        presentInfo.setImageIndices(imageIndex);
        presentInfo.setWaitSemaphores(waitSemaphores);

        auto result = m_logicDevice.GetFirstPresentQueue()->GetHandle().presentKHR(presentInfo);
        m_logicDevice.GetFirstPresentQueue()->WaitIdle();
    }
}
