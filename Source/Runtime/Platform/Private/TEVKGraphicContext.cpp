#include <vector>
#include "TEVKGraphicContext.h"
#include "TELog.h"

namespace TE {
	const std::vector<DeviceFeature> TEVKRequiredDeviceFeatures = {
		{"VK_LAYER_KHRONOS_validation", true}
	};

	const std::vector<DeviceFeature> TEVKRequiredExtension = {
		{"VK_KHR_SURFACE_EXTENSION_NAME", true},
#ifdef TE_PLATFORM_MACOS
		{"VK_MVK_MACOS_SURFACE_EXTENSION_NAME", true}，
#endif 
		
	};

	TEVKGraphicContext::TEVKGraphicContext(TEWindow* window)
	{
	}
	TEVKGraphicContext::~TEVKGraphicContext()
	{
	}
	void TEVKGraphicContext::CreateInstance()
	{
		// 1. 构建layers
		uint32_t availableLayerCount = 0;
		CALL_VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
		std::vector<VkLayerProperties> availableLayerProperties;
		availableLayerProperties.reserve(availableLayerCount);
		CALL_VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayerProperties.data()));
		

		// 2. 构建extension
		// 3. create instance
	}
}