#include <vector>
#include "TEVKGraphicContext.h"
#include "TELog.h"
#include "vulkan/vulkan.hpp"

namespace TE {
	const std::vector<DeviceFeature> TEVKRequiredLayers = {
		{"VK_LAYER_KHRONOS_validation", true}
	};

	const std::vector<DeviceFeature> TEVKRequiredExtensions = {
		{VK_KHR_SURFACE_EXTENSION_NAME, true},
#ifdef TE_WIN32
		{VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true},
#endif

#ifdef TE_MACOS

#endif

#ifdef TE_LINUX

#endif
		
	};

	TEVKGraphicContext::TEVKGraphicContext(TEWindow* window)
	{
		CreateInstance();
	}
	TEVKGraphicContext::~TEVKGraphicContext()
	{
	}
	void TEVKGraphicContext::CreateInstance()
	{
		// 1. ����layers
		uint32_t availableLayerCount = 0;
		CALL_VK_CHECK(vk::enumerateInstanceLayerProperties(&availableLayerCount, nullptr));
		std::vector<vk::LayerProperties> availableLayerProperties;
		availableLayerProperties.resize(availableLayerCount);
		CALL_VK_CHECK(vk::enumerateInstanceLayerProperties(&availableLayerCount, availableLayerProperties.data()));

		uint32_t layersCount = 0;
		std::vector<const char*> enableLayerNames;
		if (!CheckDeviceFeatureSupport("Instance Layer", false,availableLayerCount, availableLayerProperties.data(),TEVKRequiredLayers.size(), TEVKRequiredLayers,layersCount, enableLayerNames)) {
			return;
		}

		// 2. ����extension
		uint32_t availableExtensionCount = 0;
		CALL_VK_CHECK(vk::enumerateInstanceExtensionProperties("", &availableExtensionCount, nullptr));
		std::vector<vk::ExtensionProperties> availableExtensionProperties;
		availableExtensionProperties.resize(availableExtensionCount);
		CALL_VK_CHECK(vk::enumerateInstanceExtensionProperties("", &availableExtensionCount, availableExtensionProperties.data()));

		uint32_t extensionCount = 0;
		std::vector<const char*> enableExtensionsNames;
		if (!CheckDeviceFeatureSupport("Instance Extension", true,availableExtensionCount, availableExtensionProperties.data(),
			TEVKRequiredExtensions.size(), TEVKRequiredExtensions,extensionCount, enableExtensionsNames)) {
			return;
		}

		// 3. ����vk instance
		vk::ApplicationInfo vkAppInfo;
		vkAppInfo.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
				 .setPApplicationName("ToyEngine")
				 .setApiVersion(VK_API_VERSION_1_4);

		vk::InstanceCreateInfo CreateInfo;
		CreateInfo.setPNext(nullptr)
							.setPApplicationInfo(&vkAppInfo)
							.setPEnabledExtensionNames(enableLayerNames)
							.setPEnabledExtensionNames(enableExtensionsNames);

		CALL_VK_CHECK(vk::createInstance(&CreateInfo, nullptr, &m_Instance));
		LOG_TRACE("application : {}, version {}", vkAppInfo.applicationVersion, VK_MAKE_VERSION(1, 0, 0));
		LOG_TRACE("vk instance : {}", static_cast<void *>(m_Instance));
	}
}