#pragma once
#include <memory>
#include "TEGraphicContext.h"
#include "TEVKCommon.h"


namespace TE {

	struct QueueFamilyInfo {
		int32_t queueFamilyIndex = -1;
		uint32_t queueCount;
	};

	class TEVKGraphicContext : public TEGraphicContext
	{
	public:
		TEVKGraphicContext(TEWindow* window);
		~TEVKGraphicContext() override;

		vk::Instance GetInstance() const {return m_Instance;}
		vk::SurfaceKHR GetSurface() const {return m_Surface;}
		vk::PhysicalDevice GetPhysicalDevice() const {return m_PhysicalDevice;}
		vk::PhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const {return m_PhysicalDeviceMemoryProperties;}
		const QueueFamilyInfo& GetGraphicQueueFamilyInfo() const {return m_graphicQueueFamilyInfo;}
		const QueueFamilyInfo& GetPresentQueueFamilyInfo() const {return m_presentQueueFamilyInfo;}
		bool IsSameGraphicPresentQueueFamily() const{return m_graphicQueueFamilyInfo.queueFamilyIndex == m_presentQueueFamilyInfo.queueFamilyIndex;}




		bool isShouldValidate = true;
	private:
		void CreateInstance();
		void CreateSurface(TEWindow* window);
		void SelectPhysicalDevice();
		void PrintPhysicalDeviceInfo(const vk::PhysicalDevice& device);
		int ScorePhysicalDevice(const vk::PhysicalDevice& device);

		vk::Instance m_Instance;
		vk::SurfaceKHR m_Surface;
		vk::PhysicalDevice m_PhysicalDevice;
		vk::PhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
		QueueFamilyInfo m_graphicQueueFamilyInfo;
		QueueFamilyInfo m_presentQueueFamilyInfo;
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALLVkDebugReportCallback(vk::DebugReportFlagsEXT flags,
	vk::DebugReportObjectTypeEXT objectType,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData);
}