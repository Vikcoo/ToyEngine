/*
文件用途: Vulkan 公共工具与辅助方法
- 统一封装 vk::Result 文本化、错误检查宏、Layer/Extension 的可用性检查
- 供 RHI/Vulkan 下的各模块（设备/实例/交换链/管线等）复用
*/
#pragma once
#include <iomanip>
#include <sstream>
#include <string>
#include <windows.h>
#include <vulkan/vulkan_raii.hpp>
#include "TELog.h"

// 描述想要启用的 Layer 或 Extension
// name: 名称（如 "VK_LAYER_KHRONOS_validation" / VK_KHR_SWAPCHAIN_EXTENSION_NAME）
// isRequired: 是否为强制需求（true 时未找到将视为不可用）
struct DeviceFeature
{
	const char* name;
	bool isRequired;
};

// 封装 VK 返回码检查：若 result != eSuccess，则记录错误码与源位置，便于定位
#define CALL_VK_CHECK(result) vk_check(result, __FILE__, __LINE__, __FUNCTION__)

// 将任意 Feature 类型映射到可比较的“名字”，用于统一的特性匹配流程
template<typename T>
const char* getFeatureName(const T& feature);

// LayerProperties 特化：返回 layerName
template<>
inline const char* getFeatureName<vk::LayerProperties>(const vk::LayerProperties& feature) {
    return feature.layerName;
}

// ExtensionProperties 特化：返回 extensionName
template<>
inline const char* getFeatureName<vk::ExtensionProperties>(const vk::ExtensionProperties& feature) {
    return feature.extensionName;
}

// 统一检查“请求的特性(层/扩展)”是否被“可用特性列表”满足
// 参数:
//  - label: 日志标识（便于区分是 Instance Layer/Extension 还是 Device Extension）
//  - availableFeatures: 已查询到的可用特性列表
//  - requestFeatures: 期望启用的特性（含是否必需）
//  - outEnableFeatures: 输出最终启用的特性名数组（仅匹配到的会被填充）
// 返回: 是否所有“必需”特性均被满足
template<typename FeatureType>
static bool CheckDeviceFeatureSupport(
    const std::string& label, std::vector<FeatureType>& availableFeatures,
    const std::vector<DeviceFeature>& requestFeatures, std::vector<const char*>& outEnableFeatures) {

    bool findAllRequiredFeatures = true;
    outEnableFeatures.clear();

    LOG_DEBUG("--------------------{}--------------------", label);

    for (const auto& reqFeature : requestFeatures) {
        bool isFound = false;
        std::string logContent = reqFeature.isRequired ? "required , not found" : "no required, not found";
        // 遍历可用特性，使用模板函数获取名称
        for (const auto& availFeature : availableFeatures) {
            const char* availableName = getFeatureName(availFeature);

            if (std::strcmp(availableName, reqFeature.name) == 0) {
                isFound = true;
                outEnableFeatures.push_back(availableName);
                break;
            }
        }

        if (isFound) {
            logContent = reqFeature.isRequired ? "required , found" : "no required, found";
        }

        // 更新检查结果
        findAllRequiredFeatures &= isFound || !reqFeature.isRequired;

        LOG_DEBUG("{}, {}", reqFeature.name, logContent);
    }

    LOG_DEBUG("-------------------------------------");
    return findAllRequiredFeatures;
}

// 将 vk::Result 转换为可读字符串（含常见错误的简短说明）
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

// 核心检查函数，由 CALL_VK_CHECK 宏调用：记录出错位置 + 结果码文本
static void vk_check(vk::Result result, const char* fileName, uint32_t line, const char* func) {
    if (result != vk::Result::eSuccess) {
        std::stringstream ss;
        ss << "Vulkan error: " << VKResultToString(result)
           << " at " << fileName << ":" << line
           << " in " << func;
        LOG_ERROR("{}",ss.str());
    }
}