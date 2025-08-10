#pragma once
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <optional>
#include <set>
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "TELog.h"


struct DeviceFeature
{
	const char* name;
	bool isRequired;
};

#define CALL_VK_CHECK(result) vk_check(result, __FILE__, __LINE__, __FUNCTION__)

/**
 *
 * @param label
 * @param isExtension
 * @param availableCount
 * @param available
 * @param requestedCount
 * @param requestFeatures
 * @param outEnableCount
 * @param outEnableFeatures
 * @return
 */
static bool CheckDeviceFeatureSupport(
    const std::string& label, bool isExtension,
    uint32_t availableCount, void* available,
    uint32_t requestedCount, const std::vector<DeviceFeature>& requestFeatures,
    std::vector<const char*>& outEnableFeatures) {
    bool findAllRequiredFeatures = true;
    outEnableFeatures.clear();
    LOG_DEBUG("--------------------{}--------------------", label);
    for (uint32_t i = 0; i < requestedCount; i++) {
        bool isFound = false;
        std::string logContent = requestFeatures[i].isRequired ? "required , not found" : "no required, not found";
        for (uint32_t j = 0; j < availableCount; j++) {
            const char* availableName = isExtension ? ((VkExtensionProperties*)available)[j].extensionName : ((VkLayerProperties*)available)[j].layerName;
            if (strcmp(availableName, requestFeatures[i].name) == 0) {
                isFound = true;
                outEnableFeatures.push_back(availableName);
                break;
            }
        }
        if (isFound) {
            logContent = requestFeatures[i].isRequired ? "required , found" : "no required, found";
        }

        findAllRequiredFeatures &= isFound || !requestFeatures[i].isRequired;

        LOG_DEBUG("{}, {}", requestFeatures[i].name, logContent);
    }
    LOG_DEBUG("-------------------------------------");
    return findAllRequiredFeatures;
}

static std::string VKResultToString(vk::Result result) noexcept {
    switch (result) {
        // 成功状态
        case vk::Result::eSuccess:                  return "vk::Result::eSuccess";
        case vk::Result::eNotReady:                 return "vk::Result::eNotReady";
        case vk::Result::eTimeout:                  return "vk::Result::eTimeout";
        case vk::Result::eEventSet:                 return "vk::Result::eEventSet";
        case vk::Result::eEventReset:               return "vk::Result::eEventReset";
        case vk::Result::eIncomplete:               return "vk::Result::eIncomplete";
        case vk::Result::eSuboptimalKHR:            return "vk::Result::eSuboptimalKHR (交换链次优配置)";

            // 错误状态 - 内存相关
        case vk::Result::eErrorOutOfHostMemory:     return "vk::Result::eErrorOutOfHostMemory (主机内存不足)";
        case vk::Result::eErrorOutOfDeviceMemory:   return "vk::Result::eErrorOutOfDeviceMemory (设备内存不足)";
        case vk::Result::eErrorOutOfPoolMemory:     return "vk::Result::eErrorOutOfPoolMemory (内存池不足)";
        case vk::Result::eErrorMemoryMapFailed:     return "vk::Result::eErrorMemoryMapFailed (内存映射失败)";

            // 错误状态 - 初始化与驱动
        case vk::Result::eErrorInitializationFailed: return "vk::Result::eErrorInitializationFailed (初始化失败)";
        case vk::Result::eErrorIncompatibleDriver:  return "vk::Result::eErrorIncompatibleDriver (驱动不兼容)";
        case vk::Result::eErrorDeviceLost:          return "vk::Result::eErrorDeviceLost (设备已丢失)";

            // 错误状态 - 层与扩展
        case vk::Result::eErrorLayerNotPresent:     return "vk::Result::eErrorLayerNotPresent (层不存在)";
        case vk::Result::eErrorExtensionNotPresent: return "vk::Result::eErrorExtensionNotPresent (扩展不存在)";
        case vk::Result::eErrorFeatureNotPresent:   return "vk::Result::eErrorFeatureNotPresent (特性不支持)";

            // 错误状态 - 交换链与表面
        case vk::Result::eErrorSurfaceLostKHR:      return "vk::Result::eErrorSurfaceLostKHR (表面已丢失)";
        case vk::Result::eErrorOutOfDateKHR:        return "vk::Result::eErrorOutOfDateKHR (交换链过期)";
        case vk::Result::eErrorNativeWindowInUseKHR: return "vk::Result::eErrorNativeWindowInUseKHR (窗口已被使用)";
        case vk::Result::eErrorIncompatibleDisplayKHR: return "vk::Result::eErrorIncompatibleDisplayKHR (显示器不兼容)";

            // 错误状态 - 其他
        case vk::Result::eErrorInvalidExternalHandle: return "vk::Result::eErrorInvalidExternalHandle (外部句柄无效)";
        case vk::Result::eErrorInvalidOpaqueCaptureAddress: return "vk::Result::eErrorInvalidOpaqueCaptureAddress (捕获地址无效)";
        case vk::Result::eErrorFragmentation:       return "vk::Result::eErrorFragmentation (内存碎片)";
        case vk::Result::eErrorInvalidShaderNV:     return "vk::Result::eErrorInvalidShaderNV (着色器无效)";
        case vk::Result::eErrorValidationFailedEXT: return "vk::Result::eErrorValidationFailedEXT (验证层错误)";

            // 未知结果码（C++17 兼容的格式化）
        default: {
            std::stringstream ss;
            ss << "UNKNOWN_vk::Result (0x"
               << std::hex << std::setw(8) << std::setfill('0')  // 补零至 8 位十六进制
               << static_cast<int32_t>(result)
               << ")";
            return ss.str();
        }
    }
}


static void vk_check(vk::Result result, const char* fileName, uint32_t line, const char* func) {
    if (result != vk::Result::eSuccess) {
        std::stringstream ss;
        ss << "Vulkan error: " << VKResultToString(result)
           << " at " << fileName << ":" << line
           << " in " << func;
        LOG_ERROR(ss.str());
    }
}