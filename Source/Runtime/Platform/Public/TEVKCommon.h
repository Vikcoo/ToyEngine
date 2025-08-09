#pragma once
#include "vulkan/vulkan.h"
#include "TELog.h"


struct DeviceFeature
{
	const char* name;
	bool isRequired;
};

#define CALL_VK_CHECK(result) vk_check(result, __FILE__, __LINE__, __FUNCTION__)

/// <summary>
/// 检查设备特性支持情况，并输出已启用的特性数量和名称。
/// </summary>
/// <param name="label">用于标识设备特性检查的标签。</param>
/// <param name="isExtension">指示是否检查扩展特性（true 为扩展，false 为核心特性）。</param>
/// <param name="availableCount">可用特性的数量。</param>
/// <param name="available">指向可用特性列表的指针。</param>
/// <param name="requestedCount">请求启用的特性数量。</param>
/// <param name="requestFeatures">指向请求启用的设备特性数组的指针。</param>
/// <param name="outEnableCount">指向输出已启用特性数量的指针。</param>
/// <param name="outEnableFeatures">指向输出已启用特性名称的指针。</param>
/// <returns>如果所有请求的特性都被支持，则返回 true；否则返回 false。</returns>
static bool CheckDeviceFeatureSupport(
    const char* label, bool isExtension, 
    uint32_t availableCount, void* available, 
    uint32_t requestedCount, const DeviceFeature* requestFeatures,
    uint32_t* outEnableCount, const char* outEnableFeatures) {

}



static std::string VKResultToString(VkResult result) {
    switch (result) {
        // 成功状态码
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";  // 交换链次优配置

        // 错误状态码（主机内存相关）
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";

        // 层和扩展相关错误
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";

        // 实例和设备相关错误
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";  // 设备已丢失（如驱动崩溃）
    //case VK_ERROR_DEVICE_ALREADY_EXISTS: return "VK_ERROR_DEVICE_ALREADY_EXISTS";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";  // 交换链过期

        // 内存相关错误
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    //case VK_ERROR_INVALID_MEMORY_ALLOCATE: return "VK_ERROR_INVALID_MEMORY_ALLOCATE";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";

        // 资源相关错误
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";  // 内存碎片导致分配失败
    //case VK_ERROR_INVALID_PHYSICAL_DEVICE: return "VK_ERROR_INVALID_PHYSICAL_DEVICE";

        // 管道和着色器相关错误
    //case VK_ERROR_INVALID_PIPELINE_CACHE_UUID: return "VK_ERROR_INVALID_PIPELINE_CACHE_UUID";
    case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";

        // 表面和交换链相关错误
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";  // 表面已丢失
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    //case VK_ERROR_INVALID_DISPLAY_KHR: return "VK_ERROR_INVALID_DISPLAY_KHR";
    //case VK_ERROR_DISPLAY_IN_USE_KHR: return "VK_ERROR_DISPLAY_IN_USE_KHR";

        // 其他错误
    //case VK_ERROR_INVALID_COMMAND_BUFFER: return "VK_ERROR_INVALID_COMMAND_BUFFER";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";  // 验证层错误

        // 未定义的结果码
    default: return "UNKNOWN_VK_RESULT (0x" + std::to_string(result) + ")";
    }
}

static void vk_check(VkResult result, const char* fileName, uint32_t line, const char* func) {
    if (result != VK_SUCCESS) {
		LOG_ERROR("Vulkan error: " + VKResultToString(result) + " at " + fileName + ":" + std::to_string(line) + " in " + func);
    }
}