/*
文件用途: 逻辑设备实现
- 依据上下文中的物理设备/队列族，创建 vk::Device 并获取图形/呈现队列
- 同时创建 PipelineCache 以加速后续管线创建
*/
#include "Vulkan/TEVKLogicDevice.h"
#include "TELog.h"
#include "Vulkan/TEVKGraphicContext.h"
#include "Vulkan/TEVKQueue.h"
#include "vulkan/vulkan_win32.h"


namespace TE {
    // 设备级扩展需求：至少需要 Swapchain 扩展
    const std::vector<DeviceFeature> TEVKRequiredExtensions = {
        {vk::KHRSwapchainExtensionName, true},
#ifdef TE_WIN32

#elif  TE_MACOS
           {VK_KHR_portability_subset, true},
#elif  TE_LINUX
           {VK_KHR_XCB_SURFACE_EXTENSION_NAME, true}
#endif
    };

    // 创建管线缓存：用于跨管线复用编译结果，加速创建
    void TEVKLogicDevice::CreatePipelineCache() {
        vk::PipelineCacheCreateInfo pipelineCacheCreateInfo;
        m_pipelineCache = m_handle.createPipelineCache(pipelineCacheCreateInfo);
    }

    // 构造: 组装队列创建信息 -> 检查并启用设备扩展 -> 创建逻辑设备 -> 获取队列 -> 创建 PipelineCache
    TEVKLogicDevice::TEVKLogicDevice(TEVKGraphicContext &context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const TEVKSetting &setting) :m_setting(setting){

        QueueFamilyInfo graphicQueueFamilyInfo = context.GetGraphicQueueFamilyInfo();
        QueueFamilyInfo presentQueueFamilyInfo = context.GetPresentQueueFamilyInfo();
        if (graphicQueueCount == 0 || graphicQueueCount > graphicQueueFamilyInfo.queueCount) {
            LOG_ERROR("Invalid graphic queue count");
            return;
        }
        if (presentQueueCount == 0 || presentQueueCount > presentQueueFamilyInfo.queueCount) {
            LOG_ERROR("Invalid present queue count");
            return;
        }

        // 4. 配置队列优先级（同一队列族时避免索引重叠）
        const bool isSameGraphicPresentQueueFamily = context.IsSameGraphicPresentQueueFamily();
        std::vector<float> queuePriorities;
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        if (isSameGraphicPresentQueueFamily) {
            // 同一队列族：图形队列索引 [0, graphicQueueCount)，呈现队列索引 [graphicQueueCount, total)
            const uint32_t totalQueueCount = graphicQueueCount + presentQueueCount;
            if (totalQueueCount > graphicQueueFamilyInfo.queueCount) {
                LOG_ERROR("Total queues ({}) exceed max queue count ({}) for same family",
                          totalQueueCount, graphicQueueFamilyInfo.queueCount);
            }

            // 优先级：呈现队列（1.0）优先于图形队列（0.5），按索引顺序排列
            queuePriorities.reserve(totalQueueCount);
            for (uint32_t i = 0; i < presentQueueCount; ++i) {
                queuePriorities.push_back(1.0f);
            }
            for (uint32_t i = 0; i < graphicQueueCount; ++i) {
                queuePriorities.push_back(0.5f);
            }

            queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(graphicQueueFamilyInfo.queueFamilyIndex)
                .setQueueCount(totalQueueCount)
                .setQueuePriorities(queuePriorities));
        } else {
            // 不同队列族：分别配置
            std::vector<float> graphicPriorities(graphicQueueCount, 0.5f);
            queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(graphicQueueFamilyInfo.queueFamilyIndex)
                .setQueueCount(graphicQueueCount)
                .setQueuePriorities(graphicPriorities));

            std::vector<float> presentPriorities(presentQueueCount, 1.0f);
            queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(presentQueueFamilyInfo.queueFamilyIndex)
                .setQueueCount(presentQueueCount)
                .setQueuePriorities(presentPriorities));
        }

        std::vector<vk::ExtensionProperties> availableExtensionProperties = context.GetPhysicalDevice().enumerateDeviceExtensionProperties();
        std::vector<const char*> enableExtensionsNames;
        if (!CheckDeviceFeatureSupport("设备扩展 Device Extension",  availableExtensionProperties,TEVKRequiredExtensions, enableExtensionsNames)) {
            LOG_ERROR(" No TEVKRequiredExtensions");
            return;
        }

        // 6. 配置设备创建信息（补充设备功能）
        vk::PhysicalDeviceFeatures deviceFeatures{};
        //deviceFeatures.samplerAnisotropy = VK_TRUE; // 启用各向异性过滤

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.setQueueCreateInfoCount(isSameGraphicPresentQueueFamily ? 1 : 2)
                        .setQueueCreateInfos(queueCreateInfos)
                        .setPEnabledExtensionNames(enableExtensionsNames)
                        .setPEnabledFeatures(&deviceFeatures);

        // 7. 创建逻辑设备
        m_handle = context.GetPhysicalDevice().createDevice(deviceCreateInfo);
        if (m_handle == nullptr) {
            LOG_ERROR("Logical device handle is invalid!");
        }
        LOG_TRACE("逻辑设备 logic device:{} ", reinterpret_cast<uint64_t>(static_cast<void*>(*m_handle)));

        // 8. 获取图形队列（处理同一队列族的索引偏移）
        for (uint32_t i = 0; i < graphicQueueCount; ++i) {
            const uint32_t queueIndex = isSameGraphicPresentQueueFamily ?
                (i + presentQueueCount) : i; // 同一队列族时偏移呈现队列数量
            try {
                vk::raii::Queue queue = m_handle.getQueue(graphicQueueFamilyInfo.queueFamilyIndex, queueIndex);
                m_graphicQueue.push_back(std::make_shared<TEVKQueue>(
                    graphicQueueFamilyInfo.queueFamilyIndex, queueIndex, queue, false
                ));
            } catch (const vk::SystemError& e) {
                LOG_ERROR("Failed to get graphic queue {}: {}", i, e.what());
                throw;
            }
        }

        // 9. 获取呈现队列（处理同一队列族的索引偏移）
        for (uint32_t i = 0; i < presentQueueCount; ++i) {
            const uint32_t queueIndex = isSameGraphicPresentQueueFamily ? i : i; // 同一队列族时从0开始
            try {
                vk::raii::Queue queue = m_handle.getQueue(presentQueueFamilyInfo.queueFamilyIndex, queueIndex);
                m_presentQueue.push_back(std::make_shared<TEVKQueue>(
                    presentQueueFamilyInfo.queueFamilyIndex, queueIndex, queue, true
                ));
            } catch (const vk::SystemError& e) {
                LOG_ERROR("Failed to get present queue {}: {}", i, e.what());
                throw;
            }
        }

        CreatePipelineCache();
    }
}