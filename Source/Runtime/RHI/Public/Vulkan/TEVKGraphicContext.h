/*
文件用途: Vulkan 图形上下文(Instance/Surface/PhysicalDevice)管理
- 负责: 创建 Instance、创建 Surface、选择物理设备、查询/缓存队列族信息与内存属性
- 提供: RAII 句柄与必要查询接口，供后续逻辑设备/交换链等模块使用
*/
#pragma once
#include <memory>

#include "TEVKCommon.h"
#include "TEGraphicContext.h"

namespace TE {

	// 队列族信息：记录族索引与该族支持的队列数量
	struct QueueFamilyInfo {
		int32_t queueFamilyIndex = -1;
		uint32_t queueCount;
	};

	// Vulkan 图形上下文具体实现（继承通用 TEGraphicContext 抽象）
	class TEVKGraphicContext : public TEGraphicContext
	{
	public:
		explicit TEVKGraphicContext(TEWindow& window);

		// 基础查询接口：提供 RAII 封装的 Vulkan 句柄
		[[nodiscard]] const vk::raii::Instance& GetInstance() const {return m_instance;}
		[[nodiscard]] const vk::raii::SurfaceKHR& GetSurface() const {return m_surface;}
		[[nodiscard]] const vk::raii::PhysicalDevice& GetPhysicalDevice() const {return m_physicalDevice;}
		[[nodiscard]] vk::PhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const {return m_PhysicalDeviceMemoryProperties;}
		[[nodiscard]] const QueueFamilyInfo& GetGraphicQueueFamilyInfo() const {return m_graphicQueueFamilyInfo;}
		[[nodiscard]] const QueueFamilyInfo& GetPresentQueueFamilyInfo() const {return m_presentQueueFamilyInfo;}
		[[nodiscard]] bool IsSameGraphicPresentQueueFamily() const{return m_graphicQueueFamilyInfo.queueFamilyIndex == m_presentQueueFamilyInfo.queueFamilyIndex;}

		// Debug Messenger 创建信息与回调（供 VK_EXT_debug_utils 使用）
		static vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,vk::DebugUtilsMessageTypeFlagsEXT messageTypes,vk::DebugUtilsMessengerCallbackDataEXT const * pCallbackData,void * pUserData);

		// 是否开启校验（示例开关，可用于控制是否启用校验层/调试输出）
		bool isShouldValidate = true;
	private:
		// 创建 Vulkan Instance（组装 ApplicationInfo、Layer/Extension、DebugMessenger）
		void CreateInstance();
		// 为当前窗口创建表面（GLFW 或平台特定接口）
		void CreateSurface(TEWindow& window);
		// 选择合适的物理设备，并解析图形/呈现队列族
		void SelectPhysicalDevice();
		// 打印物理设备的关键信息（特性/内存/队列/扩展）
		void PrintPhysicalDeviceInfo(const vk::PhysicalDevice& device);
		// 对物理设备打分，便于在多设备环境中选择最佳设备
		static int ScorePhysicalDevice(const vk::PhysicalDevice& device);



		vk::raii::Context m_context;
		vk::raii::Instance m_instance{ nullptr };
		vk::raii::SurfaceKHR m_surface{ nullptr };
		vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
		vk::PhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
		QueueFamilyInfo m_graphicQueueFamilyInfo;
		QueueFamilyInfo m_presentQueueFamilyInfo;
	};
}