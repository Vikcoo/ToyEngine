#pragma once
#include <memory>

#include "TEVKCommon.h"
#include "TEGraphicContext.h"

namespace TE {

	struct QueueFamilyInfo {
		int32_t queueFamilyIndex = -1;
		uint32_t queueCount;
	};

	class TEVKGraphicContext : public TEGraphicContext
	{
	public:
		explicit TEVKGraphicContext(TEWindow& window);

		[[nodiscard]] const vk::raii::Instance& GetInstance() const {return m_instance;}
		[[nodiscard]] const vk::raii::SurfaceKHR& GetSurface() const {return m_surface;}
		[[nodiscard]] const vk::raii::PhysicalDevice& GetPhysicalDevice() const {return m_physicalDevice;}
		[[nodiscard]] vk::PhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const {return m_PhysicalDeviceMemoryProperties;}
		[[nodiscard]] const QueueFamilyInfo& GetGraphicQueueFamilyInfo() const {return m_graphicQueueFamilyInfo;}
		[[nodiscard]] const QueueFamilyInfo& GetPresentQueueFamilyInfo() const {return m_presentQueueFamilyInfo;}
		[[nodiscard]] bool IsSameGraphicPresentQueueFamily() const{return m_graphicQueueFamilyInfo.queueFamilyIndex == m_presentQueueFamilyInfo.queueFamilyIndex;}

		static vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,vk::DebugUtilsMessageTypeFlagsEXT messageTypes,vk::DebugUtilsMessengerCallbackDataEXT const * pCallbackData,void * pUserData);

		bool isShouldValidate = true;
	private:
		void CreateInstance();
		void CreateSurface(TEWindow& window);
		void SelectPhysicalDevice();
		void PrintPhysicalDeviceInfo(const vk::PhysicalDevice& device);
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