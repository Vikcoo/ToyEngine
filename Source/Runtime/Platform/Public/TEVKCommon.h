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
/// ����豸����֧�����������������õ��������������ơ�
/// </summary>
/// <param name="label">���ڱ�ʶ�豸���Լ��ı�ǩ��</param>
/// <param name="isExtension">ָʾ�Ƿ�����չ���ԣ�true Ϊ��չ��false Ϊ�������ԣ���</param>
/// <param name="availableCount">�������Ե�������</param>
/// <param name="available">ָ����������б��ָ�롣</param>
/// <param name="requestedCount">�������õ�����������</param>
/// <param name="requestFeatures">ָ���������õ��豸���������ָ�롣</param>
/// <param name="outEnableCount">ָ���������������������ָ�롣</param>
/// <param name="outEnableFeatures">ָ������������������Ƶ�ָ�롣</param>
/// <returns>���������������Զ���֧�֣��򷵻� true�����򷵻� false��</returns>
static bool CheckDeviceFeatureSupport(
    const char* label, bool isExtension, 
    uint32_t availableCount, void* available, 
    uint32_t requestedCount, const DeviceFeature* requestFeatures,
    uint32_t* outEnableCount, const char* outEnableFeatures) {

}



static std::string VKResultToString(VkResult result) {
    switch (result) {
        // �ɹ�״̬��
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";  // ��������������

        // ����״̬�루�����ڴ���أ�
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";

        // �����չ��ش���
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";

        // ʵ�����豸��ش���
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";  // �豸�Ѷ�ʧ��������������
    //case VK_ERROR_DEVICE_ALREADY_EXISTS: return "VK_ERROR_DEVICE_ALREADY_EXISTS";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";  // ����������

        // �ڴ���ش���
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    //case VK_ERROR_INVALID_MEMORY_ALLOCATE: return "VK_ERROR_INVALID_MEMORY_ALLOCATE";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";

        // ��Դ��ش���
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";  // �ڴ���Ƭ���·���ʧ��
    //case VK_ERROR_INVALID_PHYSICAL_DEVICE: return "VK_ERROR_INVALID_PHYSICAL_DEVICE";

        // �ܵ�����ɫ����ش���
    //case VK_ERROR_INVALID_PIPELINE_CACHE_UUID: return "VK_ERROR_INVALID_PIPELINE_CACHE_UUID";
    case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";

        // ����ͽ�������ش���
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";  // �����Ѷ�ʧ
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    //case VK_ERROR_INVALID_DISPLAY_KHR: return "VK_ERROR_INVALID_DISPLAY_KHR";
    //case VK_ERROR_DISPLAY_IN_USE_KHR: return "VK_ERROR_DISPLAY_IN_USE_KHR";

        // ��������
    //case VK_ERROR_INVALID_COMMAND_BUFFER: return "VK_ERROR_INVALID_COMMAND_BUFFER";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";  // ��֤�����

        // δ����Ľ����
    default: return "UNKNOWN_VK_RESULT (0x" + std::to_string(result) + ")";
    }
}

static void vk_check(VkResult result, const char* fileName, uint32_t line, const char* func) {
    if (result != VK_SUCCESS) {
		LOG_ERROR("Vulkan error: " + VKResultToString(result) + " at " + fileName + ":" + std::to_string(line) + " in " + func);
    }
}