#include <vector>
#include "TEVKGraphicContext.h"

#include <optional>
#include <set>

#include "TEGLFWWindow.h"
#include "TELog.h"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_win32.h"
#include "unordered_set"

namespace TE {
	const std::vector<DeviceFeature> TEVKRequiredLayers = {
		{"VK_LAYER_KHRONOS_validation", true},
	};

	const std::vector<DeviceFeature> TEVKRequiredExtensions = {
		{VK_KHR_SURFACE_EXTENSION_NAME, true},
#ifdef TE_WIN32
		{VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true},
#elif  TE_MACOS
		   {VK_MVK_MACOS_SURFACE_EXTENSION_NAME, true},
#elif  TE_LINUX
		   {VK_KHR_XCB_SURFACE_EXTENSION_NAME, true}
#endif
	};

	TEVKGraphicContext::TEVKGraphicContext(TEWindow* window)
	{
		CreateInstance();
		CreateSurface(window);
		SelectPhysicalDevice();
	}

	TEVKGraphicContext::~TEVKGraphicContext()
	{
		if (m_Instance) {
			if (m_Surface) {  // 检查是否有效
				m_Instance.destroySurfaceKHR(m_Surface);
				m_Surface = nullptr;
			}
			m_Instance.destroy();
			m_Instance = nullptr;
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugReportCallback(vk::DebugReportFlagsEXT flags,
															    vk::DebugReportObjectTypeEXT objectType,
															    uint64_t object,
															    size_t location,
															    int32_t messageCode,
															    const char* pLayerPrefix,
															    const char* pMessage,
															    void* pUserData) {
								if (flags & vk::DebugReportFlagBitsEXT::eError) {
									LOG_ERROR(pMessage);
								}

								if (flags & vk::DebugReportFlagBitsEXT::eWarning || flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning) {
									LOG_WARN(pMessage);
								}

								if (flags & vk::DebugReportFlagBitsEXT::eDebug) {
									LOG_DEBUG(pMessage);
								}

								if (flags & vk::DebugReportFlagBitsEXT::eInformation) {
									LOG_INFO(pMessage);
								}
								return VK_FALSE;
							}

	void TEVKGraphicContext::CreateInstance()
	{
		// 1. 构建layers
		uint32_t availableLayerCount = 0;
		CALL_VK_CHECK(vk::enumerateInstanceLayerProperties(&availableLayerCount, nullptr));
		std::vector<vk::LayerProperties> availableLayerProperties;
		availableLayerProperties.resize(availableLayerCount);
		CALL_VK_CHECK(vk::enumerateInstanceLayerProperties(&availableLayerCount, availableLayerProperties.data()));

		std::vector<const char*> enableLayerNames;
		if (isShouldValidate) {
			if (!CheckDeviceFeatureSupport("Instance Layer", false,availableLayerCount, availableLayerProperties.data(),TEVKRequiredLayers.size(), TEVKRequiredLayers, enableLayerNames)) {
				return;
			}
		}

		// 2. 构建extension
		uint32_t availableExtensionCount = 0;
		CALL_VK_CHECK(vk::enumerateInstanceExtensionProperties("", &availableExtensionCount, nullptr));
		std::vector<vk::ExtensionProperties> availableExtensionProperties;
		availableExtensionProperties.resize(availableExtensionCount);
		CALL_VK_CHECK(vk::enumerateInstanceExtensionProperties("", &availableExtensionCount, availableExtensionProperties.data()));

		std::vector<const char*> enableExtensionsNames;
		if (!CheckDeviceFeatureSupport("Instance Extension", true,availableExtensionCount, availableExtensionProperties.data(),
			TEVKRequiredExtensions.size(), TEVKRequiredExtensions,enableExtensionsNames)) {
			return;
		}

		// 3. 构建vk instance
		vk::ApplicationInfo vkAppInfo;
		vkAppInfo.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
				 .setPApplicationName("ToyEngine")
				 .setApiVersion(VK_API_VERSION_1_4);

		vk::DebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo;
		debugReportCallbackCreateInfo.setPfnCallback(isShouldValidate ? VkDebugReportCallback : nullptr)
									 .setFlags(vk::DebugReportFlagBitsEXT::eError |
											   vk::DebugReportFlagBitsEXT::eWarning |
											   vk::DebugReportFlagBitsEXT::ePerformanceWarning |
											   vk::DebugReportFlagBitsEXT::eDebug |
											   vk::DebugReportFlagBitsEXT::eInformation);

		vk::InstanceCreateInfo CreateInfo;
		CreateInfo.setPNext(nullptr)
				  .setPApplicationInfo(&vkAppInfo)
				  .setPEnabledExtensionNames(enableLayerNames)
				  .setPEnabledExtensionNames(enableExtensionsNames)
				  .setPNext(debugReportCallbackCreateInfo);

		CALL_VK_CHECK(vk::createInstance(&CreateInfo, nullptr, &m_Instance));
		LOG_TRACE("application : {}, version {}", vkAppInfo.applicationVersion, VK_MAKE_VERSION(1, 0, 0));
		LOG_TRACE("vk instance : {}", static_cast<void *>(m_Instance));
	}

	void TEVKGraphicContext::CreateSurface(TEWindow* window) {
		if (!window) {
			LOG_ERROR("window is null");
			return;
		}

		// todo:可能有除了glfw的
		auto* glfwWindow = dynamic_cast<TEGLFWWindow *>(window);
		if (!glfwWindow) {
			LOG_ERROR("glfwWindow is null");
			return;
		}

		VkSurfaceKHR rawSurface;
		if (glfwCreateWindowSurface(m_Instance, glfwWindow->GetGLFWWindowHandle(), nullptr, &rawSurface) != VK_SUCCESS) {
			LOG_ERROR("Failed to create window surface!");
		}
		m_Surface = rawSurface;
		LOG_INFO("m_Surface : {}", static_cast<void *>(m_Surface));
	}

	void TEVKGraphicContext::SelectPhysicalDevice() {
		std::vector<vk::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();
		LOG_INFO("physical devices count : {}", physicalDevices.size());

		if (physicalDevices.empty()) {
			LOG_ERROR("Failed to find a suitable physical device!");
			return;
		}

		int maxScore = -1;
		int maxScoreIndex = -1;
		for (int i = 0; i < physicalDevices.size(); i++) {


			int score = ScorePhysicalDevice(physicalDevices[i]);
			maxScore = maxScore > score ? maxScore : score;
			maxScoreIndex = maxScore > score ? maxScoreIndex : i;

			// 2. 遍历查找支持的队列族
			std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevices[i].getQueueFamilyProperties();
			for (int32_t j = 0; j < queueFamilies.size(); ++j) {
				// 检查图形支持（Graphics Pipeline）
				if (queueFamilies[j].queueFlags & vk::QueueFlagBits::eGraphics) {
					m_graphicQueueFamilyInfo.queueFamilyIndex = j;
					m_graphicQueueFamilyInfo.queueCount = queueFamilies[j].queueCount;
				}

				// 检查显示支持（Presentation）
				vk::Bool32 presentSupport = false;
				presentSupport = physicalDevices[i].getSurfaceSupportKHR(j, m_Surface);
				if (presentSupport) {
					m_presentQueueFamilyInfo.queueFamilyIndex = j;
					m_presentQueueFamilyInfo.queueCount = queueFamilies[j].queueCount;
				}

				// 提前退出优化：如果图形和显示是同一队列族
				if (m_graphicQueueFamilyInfo.queueFamilyIndex == m_presentQueueFamilyInfo.queueFamilyIndex) {
					break;
				}
			}

		}

		if (maxScoreIndex == -1) {
			LOG_ERROR("Failed to find a suitable physical device!");
			maxScoreIndex = 0;
		}

		m_PhysicalDevice = physicalDevices[maxScoreIndex];
		m_PhysicalDeviceMemoryProperties = physicalDevices[maxScoreIndex].getMemoryProperties();
		PrintPhysicalDeviceInfo(m_PhysicalDevice);
		LOG_TRACE("selected physical device : {}  , Score : {}", static_cast<void *>(m_PhysicalDevice), maxScore);
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
}
