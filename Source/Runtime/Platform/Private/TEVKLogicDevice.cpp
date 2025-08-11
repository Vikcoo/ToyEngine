//
// Created by yukai on 2025/8/10.
//
#include "TEVKLogicDevice.h"
#include "TELog.h"
#include "TEVKGraphicContext.h"
#include "TEVKQueue.h"
#include "vulkan/vulkan_win32.h"


namespace TE {
    const std::vector<DeviceFeature> TEVKRequiredExtensions = {
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME, true},
#ifdef TE_WIN32

#elif  TE_MACOS
           {VK_KHR_portability_subset, true},
#elif  TE_LINUX
           {VK_KHR_XCB_SURFACE_EXTENSION_NAME, true}
#endif
    };



    TEVKLogicDevice::~TEVKLogicDevice() {
        m_handle.waitIdle();
        m_handle.destroy();
    }

    TEVKLogicDevice::TEVKLogicDevice(TEVKGraphicContext *context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const TEVKSetting &setting) {
        if (!context) {
            LOG_ERROR(" No TEVKGraphicContext");
        }

        QueueFamilyInfo graphicQueueFamilyInfo = context->GetGraphicQueueFamilyInfo();
        QueueFamilyInfo presentQueueFamilyInfo = context->GetPresentQueueFamilyInfo();

        if (graphicQueueCount > graphicQueueFamilyInfo.queueCount) {
            LOG_ERROR(" graphicQueueCount > queueGraphicFamilyInfo.queueCount");
            return;
        }
        if (presentQueueCount > presentQueueFamilyInfo.queueCount) {
            LOG_ERROR(" presentQueueCount > queuePresentFamilyInfo.queueCount");
            return;
        }

        std::vector<float> graphicQueuePriorities(graphicQueueCount, 0);
        std::vector<float> presentQueuePriorities(presentQueueCount, 1);


        bool isSameGraphicPresentQueueFamily = context->IsSameGraphicPresentQueueFamily();
        uint32_t sameQueueCount = graphicQueueCount;
        if (isSameGraphicPresentQueueFamily) {
            sameQueueCount += presentQueueCount;
            if (sameQueueCount > graphicQueueFamilyInfo.queueCount) {
                sameQueueCount = graphicQueueFamilyInfo.queueCount;
            }
            graphicQueuePriorities.insert(graphicQueuePriorities.begin(), presentQueuePriorities.begin(), presentQueuePriorities.end());
        }
        // 2. 配置队列创建信息
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo().setQueueFamilyIndex(graphicQueueFamilyInfo.queueFamilyIndex)
                                                                .setQueueCount(sameQueueCount)
                                                                .setQueuePriorities(graphicQueuePriorities));
        if (!isSameGraphicPresentQueueFamily) {
            queueCreateInfos.push_back(vk::DeviceQueueCreateInfo().setQueueFamilyIndex(presentQueueFamilyInfo.queueFamilyIndex)
                                                                    .setQueueCount(presentQueueCount)
                                                                    .setQueuePriorities(presentQueuePriorities));
        }

        std::vector<vk::ExtensionProperties> availableExtensionProperties = context->GetPhysicalDevice().enumerateDeviceExtensionProperties();
        std::vector<const char*> enableExtensionsNames;
        if (!CheckDeviceFeatureSupport("设备扩展 Device Extension", true,availableExtensionProperties.size(), availableExtensionProperties.data(),
            TEVKRequiredExtensions.size(), TEVKRequiredExtensions, enableExtensionsNames)) {
            LOG_ERROR(" No TEVKRequiredExtensions");
                return;
            }

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.setQueueCreateInfoCount(isSameGraphicPresentQueueFamily ? 1 : 2)
                        .setQueueCreateInfos(queueCreateInfos)
                        .setPEnabledExtensionNames(enableExtensionsNames);

        m_handle = context->GetPhysicalDevice().createDevice(deviceCreateInfo);
        LOG_TRACE("逻辑设备 logic device:{} ", (void*) m_handle);
        for (uint32_t i = 0; i < graphicQueueCount; i++) {
            vk::Queue queue = m_handle.getQueue(graphicQueueFamilyInfo.queueFamilyIndex, i);
             m_graphicQueue.push_back(std::make_shared<TEVKQueue>(graphicQueueFamilyInfo.queueFamilyIndex, i, queue, false));
        }
        for (uint32_t i = 0; i < presentQueueCount; i++) {
            vk::Queue queue = m_handle.getQueue(presentQueueFamilyInfo.queueFamilyIndex, i);
            m_presentQueue.push_back(std::make_shared<TEVKQueue>(presentQueueFamilyInfo.queueFamilyIndex, i, queue, true));
        }
    }
}
