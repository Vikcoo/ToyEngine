#include "Vulkan/TEVKGraphicContext.h"
#include <vector>

#include <optional>
#include <set>

#include "TEGLFWWindow.h"
#include "TELog.h"
#include "TEVKCommon.h"
#include "unordered_set"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_win32.h"

namespace TE {
	const std::vector<DeviceFeature> TEVKRequiredLayers = {
		{"VK_LAYER_KHRONOS_validation", true},
	};

	const std::vector<DeviceFeature> TEVKRequiredExtensions = {
		{VK_KHR_SURFACE_EXTENSION_NAME, true},
{VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true},

#ifdef TE_WIN32
		{VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true},
#elif  TE_MACOS
		   {VK_MVK_MACOS_SURFACE_EXTENSION_NAME, true},
#elif  TE_LINUX
		   {VK_KHR_XCB_SURFACE_EXTENSION_NAME, true}
#endif
	};

	TEVKGraphicContext::TEVKGraphicContext(TEWindow& window)
	{
		CreateInstance();
		CreateSurface(window);
		SelectPhysicalDevice();
	}


	void TEVKGraphicContext::CreateInstance()
	{
		vk::ApplicationInfo applicationInfo(
			"Hello Triangle",   									// pApplicationName
			1,                  									// applicationVersion
			"No Engine",        									// pEngineName
			1,                  									// engineVersion
			vk::makeApiVersion(0, 1, 4, 0)   // apiVersion
		);

		// 1. 构建layers
		std::vector<vk::LayerProperties> availableLayerProperties = vk::enumerateInstanceLayerProperties();
		std::vector<const char*> enableLayerNames;

		if (!CheckDeviceFeatureSupport("Instance Layer", availableLayerProperties, TEVKRequiredLayers, enableLayerNames)) {
			return;
		}
		if (enableLayerNames.empty()) LOG_ERROR("No Layers available");


		// 2. 构建extension
		std::vector<vk::ExtensionProperties> availableExtensionProperties = vk::enumerateInstanceExtensionProperties();
		std::vector<const char*> enableExtensionsNames;
		if (!CheckDeviceFeatureSupport("Instance Extension",  availableExtensionProperties, TEVKRequiredExtensions,enableExtensionsNames)) {
			return;
		}
		if (enableExtensionsNames.empty()) LOG_ERROR("No extensions available");

		// 3. 构建vk instance、
		auto debugMessengerCreateInfo = populateDebugMessengerCreateInfo();
		vk::InstanceCreateInfo createInfo;
		createInfo.setPApplicationInfo(&applicationInfo);
		createInfo.setPEnabledLayerNames(enableLayerNames);
		createInfo.setPEnabledExtensionNames(enableExtensionsNames);
		createInfo.setPNext(&debugMessengerCreateInfo);

		m_instance = m_context.createInstance(createInfo);
		LOG_TRACE("application : {}", applicationInfo.applicationVersion);
	}

	void TEVKGraphicContext::CreateSurface(TEWindow& window) {

		// todo:可能有除了glfw的
		auto* glfwWindow = dynamic_cast<TEGLFWWindow *>(&window);
		if (!glfwWindow) {
			LOG_ERROR("glfwWindow 为空");
			return;
		}

		VkSurfaceKHR cSurface;
		if (glfwCreateWindowSurface(*m_instance, glfwWindow->GetGLFWWindowHandle(), nullptr, &cSurface) != VK_SUCCESS) {
			LOG_ERROR("创建 surface 失败!");
			return;
		}
		m_surface = vk::raii::SurfaceKHR(m_instance, cSurface);
		LOG_INFO("m_Surface : {}",reinterpret_cast<uint64_t>(static_cast<void*>(*m_surface)));
	}

	void TEVKGraphicContext::SelectPhysicalDevice() {
		const auto physicalDevices = m_instance.enumeratePhysicalDevices();
        LOG_TRACE("物理设备数 : {}", physicalDevices.size());
		if (physicalDevices.empty()) {
			LOG_ERROR("没有找到合适的物理设备!");
			return;
		}

		int maxScore = -1;
		vk::raii::PhysicalDevice selectedDevice{nullptr};
		QueueFamilyInfo bestGraphicQueue{};
		QueueFamilyInfo bestPresentQueue{};
		for (const auto& device : physicalDevices) {
			const int score = ScorePhysicalDevice(device);
			QueueFamilyInfo graphicQueue{};
			QueueFamilyInfo presentQueue{};
			bool hasValidQueues = false;

			auto queueFamilies = device.getQueueFamilyProperties();
			for (uint32_t j = 0; j < queueFamilies.size(); ++j) {
				const auto& queueFamily = queueFamilies[j];

				// 检查图形队列
				if (graphicQueue.queueFamilyIndex == -1 && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
					graphicQueue.queueFamilyIndex = static_cast<int32_t>(j);
					graphicQueue.queueCount = queueFamily.queueCount;
				}

				// 检查呈现队列：使用 RAII 表面的原始句柄（*m_surface）
				if (presentQueue.queueFamilyIndex == -1) {
					vk::Bool32 presentSupport; // 直接使用原始句柄调用 C 函数，避免类型转换问题
					vkGetPhysicalDeviceSurfaceSupportKHR(
						*device,					// 原始物理设备句柄
						j,                          // 队列族索引
						*m_surface,              // 原始表面句柄（VkSurfaceKHR）
						&presentSupport             // 输出是否支持
					);

					if (presentSupport) {
						presentQueue.queueFamilyIndex = static_cast<int32_t>(j);
						presentQueue.queueCount = queueFamily.queueCount;
					}
				}

				if (graphicQueue.queueFamilyIndex != -1 && presentQueue.queueFamilyIndex != -1) {
					hasValidQueues = true;
					if (graphicQueue.queueFamilyIndex == presentQueue.queueFamilyIndex) break;
				}
			}

			if (hasValidQueues && score > maxScore) {
				maxScore = score;
				selectedDevice = device;
				bestGraphicQueue = graphicQueue;
				bestPresentQueue = presentQueue;
			}
		}

		if (selectedDevice == nullptr) {
			LOG_ERROR("没找到合适的物理设备!");
		}

		m_physicalDevice = std::move(selectedDevice);
		m_graphicQueueFamilyInfo = bestGraphicQueue;
		m_presentQueueFamilyInfo = bestPresentQueue;
		m_PhysicalDeviceMemoryProperties = m_physicalDevice.getMemoryProperties();

		PrintPhysicalDeviceInfo(m_physicalDevice);
		LOG_TRACE("选择的物理设备 : {}  , 分数 : {}", m_physicalDevice.getProperties().deviceName.data(), maxScore);
	}

	void TEVKGraphicContext::PrintPhysicalDeviceInfo(const vk::PhysicalDevice &device) {
		// 1. 获取设备属性
		vk::PhysicalDeviceProperties properties = device.getProperties();

		LOG_DEBUG("=== 物理设备信息 ===");
		LOG_DEBUG("设备名称: {}", properties.deviceName.data() );
		LOG_DEBUG("设备ID: {}", properties.deviceID);
		LOG_DEBUG("vendorID: {}", properties.vendorID);
		LOG_DEBUG("设备类型: {}", vk::to_string(properties.deviceType));
		LOG_DEBUG("驱动版本: {}", properties.driverVersion);

		uint32_t version = properties.apiVersion;
		uint32_t manualMajor = (version >> 22) & 0x3FF;
		uint32_t manualMinor = (version >> 12) & 0x3FF;
		uint32_t manualPatch = version & 0xFFF;
		LOG_DEBUG("API 版本: {}.{}.{} (原始值: {})", manualMajor, manualMinor, manualPatch, properties.apiVersion);

		// 2. 功能特性
		vk::PhysicalDeviceFeatures features = device.getFeatures();
		LOG_DEBUG("=== 支持的功能 ===");
		LOG_DEBUG("几何着色器: {}", features.geometryShader ? "是" : "否");
		LOG_DEBUG("曲面细分着色器: {}", features.tessellationShader ? "是" : "否");
		LOG_DEBUG("多视口渲染: {}", features.multiViewport ? "是" : "否");

		// 3. 内存信息
		vk::PhysicalDeviceMemoryProperties memoryProps = device.getMemoryProperties();
		LOG_DEBUG("=== 内存堆信息 ===");
		for (uint32_t i = 0; i < memoryProps.memoryHeapCount; ++i) {
			LOG_DEBUG("堆 {}: 大小 {} MB, 类型: {}",
					 i,
					 memoryProps.memoryHeaps[i].size / (1024 * 1024),
					 vk::to_string(memoryProps.memoryHeaps[i].flags));
		}

		// 4. 扩展支持
		std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();
		LOG_DEBUG("=== 支持的扩展 ({} 个) ===", extensions.size());

		// 5. 队列族
		std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
		LOG_DEBUG("=== 队列族 ({} 个) ===", queueFamilies.size());
		for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
			LOG_DEBUG("族 {}: {} 个队列, 支持操作: {}",
					i,
					queueFamilies[i].queueCount,
					vk::to_string(queueFamilies[i].queueFlags));
		}

		LOG_DEBUG("图形队列族索引: {}", m_graphicQueueFamilyInfo.queueFamilyIndex);
		LOG_DEBUG("显示队列族索引: {}", m_presentQueueFamilyInfo.queueFamilyIndex);
		if (IsSameGraphicPresentQueueFamily()) {
			LOG_WARN("注意：图形和显示共享同一队列族");
		}
	}

	int TEVKGraphicContext::ScorePhysicalDevice(const vk::PhysicalDevice& device) {
		int score = 0;
		vk::PhysicalDeviceProperties props = device.getProperties();
		vk::PhysicalDeviceFeatures features = device.getFeatures();
		vk::PhysicalDeviceMemoryProperties memoryProps = device.getMemoryProperties();

		// 1. 设备类型评分
		switch (props.deviceType) {
			case vk::PhysicalDeviceType::eDiscreteGpu:  score += 50; break;
			case vk::PhysicalDeviceType::eIntegratedGpu: score += 30; break;
			case vk::PhysicalDeviceType::eVirtualGpu:  score += 10; break;
			case vk::PhysicalDeviceType::eCpu:         score += 0;  break;
			default:                                   score += 5;  break;
		}

		// 2. 功能评分
		if (features.geometryShader)    score += 10;
		if (features.tessellationShader) score += 10;
		if (features.samplerAnisotropy)  score += 5;  // 各向异性过滤

		// 3. 显存评分（取最大本地内存堆）
		for (uint32_t i = 0; i < memoryProps.memoryHeapCount; ++i) {
			if (memoryProps.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
				score += static_cast<int>(memoryProps.memoryHeaps[i].size / (1024 * 1024 * 1024)); // GB
			}
		}

		// 4. 扩展评分
		std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();
		std::unordered_set<std::string> requiredExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME  // 示例：其他重要扩展
		};

		for (const auto& ext : extensions) {
			if (requiredExtensions.count(ext.extensionName)) {
				score += 20;  // 关键扩展
			} else {
				score += 1;   // 其他扩展
			}
		}

		return score;
	}

	// 初始化调试信使创建信息
	vk::DebugUtilsMessengerCreateInfoEXT TEVKGraphicContext::populateDebugMessengerCreateInfo() {
		vk::DebugUtilsMessengerCreateInfoEXT createInfo;
		// 显式初始化所有必要成员，避免使用不明确的初始化列表
		createInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT; // 必须设置
		createInfo.pNext = nullptr; // 无后续结构体
		createInfo.flags = {}; // 无特殊标志

		// 设置需要捕获的消息级别（示例：所有级别）
		createInfo.messageSeverity =
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

		// 设置需要捕获的消息类型（示例：所有类型）
		createInfo.messageType =
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

		// 绑定回调函数（注意：必须是兼容的函数指针）
		createInfo.pfnUserCallback = debugMessageFunc;
		createInfo.pUserData = nullptr; // 无用户数据
		return createInfo;
	}

	vk::Bool32 TEVKGraphicContext::debugMessageFunc(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,vk::DebugUtilsMessageTypeFlagsEXT messageTypes,vk::DebugUtilsMessengerCallbackDataEXT const * pCallbackData,void * pUserData) {
		if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
			LOG_WARN("{}",pCallbackData->pMessage);
		}
		return VK_FALSE;
	}
}
