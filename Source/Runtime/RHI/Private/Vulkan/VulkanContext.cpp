// Vulkan Context 实现
#include "VulkanContext.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"
#include "VulkanUtils.h"
#include "Window.h"
#include "Log/Log.h"

#include <GLFW/glfw3.h>

namespace TE {

// ============================================================================
// 工厂方法和构造/析构
// ============================================================================

std::shared_ptr<VulkanContext> VulkanContext::Create(const VulkanContextConfig& config) {
    auto context = std::make_shared<VulkanContext>(PrivateTag{}, config);

    if (!context->Initialize()) {
        TE_LOG_ERROR("Failed to initialize Vulkan context");
        return nullptr;
    }
    
    TE_LOG_INFO("Vulkan Context created successfully");
    return context;
}

VulkanContext::VulkanContext(PrivateTag, const VulkanContextConfig& config)
    : m_config(config)
{
    TE_LOG_INFO("Initializing Vulkan Context");
    TE_LOG_INFO("  App: {}", config.appName);
    TE_LOG_INFO("  Validation: {}", config.enableValidation ? "Enabled" : "Disabled");
}

VulkanContext::~VulkanContext() {
    TE_LOG_INFO("Destroying Vulkan Context");
    m_debugMessenger.clear();
    m_instance.clear();
}

// ============================================================================
// 初始化
// ============================================================================

bool VulkanContext::Initialize() {
    if (!CreateInstance()) {
        return false;
    }
    
    if (m_config.enableValidation) {
        SetupDebugMessenger();
    }
    
    return true;
}

bool VulkanContext::CreateInstance() {
    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName(m_config.appName.c_str())
           .setApplicationVersion(m_config.appVersion)
           .setPEngineName("ToyEngine")
           .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
           .setApiVersion(m_config.apiVersion);
    
    // 获取所需扩展
    const auto extensions = GetRequiredInstanceExtensions(m_config.enableValidation);
    
    if (!CheckInstanceExtensionSupport(extensions)) {
        TE_LOG_ERROR("Required extensions not supported");
        return false;
    }
    
    // 验证层
    std::vector<const char*> layers;
    if (m_config.enableValidation) {
        layers = {"VK_LAYER_KHRONOS_validation"};
        
        if (!CheckValidationLayerSupport(layers)) {
            TE_LOG_WARN("Validation layers requested but not available");
            layers.clear();
        }
    }

    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo)
              .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
              .setPpEnabledExtensionNames(extensions.data())
              .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
              .setPpEnabledLayerNames(layers.data());

    // Debugger
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (m_config.enableValidation && !layers.empty()) {
        debugCreateInfo = GetDebugMessengerCreateInfo();
        createInfo.setPNext(&debugCreateInfo);
    }
    
    // 创建 Instance
    m_instance = vk::raii::Instance(m_context, createInfo);
    TE_LOG_INFO("Vulkan Instance created");
    return true;
}

// ============================================================================
// Debug Messenger
// ============================================================================

void VulkanContext::SetupDebugMessenger() {
    const VkDebugUtilsMessengerCreateInfoEXT createInfo = GetDebugMessengerCreateInfo();
    
    try {
        m_debugMessenger = vk::raii::DebugUtilsMessengerEXT(m_instance, createInfo);
        TE_LOG_INFO("Debug Messenger setup complete");
    }
    catch (const vk::SystemError& e) {
        TE_LOG_WARN("Failed to setup Debug Messenger: {}", e.what());
    }
}

vk::DebugUtilsMessengerCreateInfoEXT VulkanContext::GetDebugMessengerCreateInfo() {
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.setMessageSeverity(
                  vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                  vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
              .setMessageType(
                  vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                  vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                  vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
              .setPfnUserCallback(&DebugCallbackCpp);
    return createInfo;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        TE_LOG_ERROR("[Vulkan] {}", pCallbackData->pMessage);
    }
    else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        TE_LOG_WARN("[Vulkan] {}", pCallbackData->pMessage);
    }
    else {
        TE_LOG_DEBUG("[Vulkan] {}", pCallbackData->pMessage);
    }
    
    return VK_FALSE;
}

VkBool32 VulkanContext::DebugCallbackCpp(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    // 使用 vulkan-hpp 风格的类型
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        TE_LOG_ERROR("[Vulkan] {}", pCallbackData->pMessage);
    }
    else if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        TE_LOG_WARN("[Vulkan] {}", pCallbackData->pMessage);
    }
    else {
        TE_LOG_DEBUG("[Vulkan] {}", pCallbackData->pMessage);
    }
    
    return VK_FALSE;
}

// ============================================================================
// 物理设备和 Surface
// ============================================================================

std::vector<std::shared_ptr<VulkanPhysicalDevice>> VulkanContext::EnumeratePhysicalDevices() {
    vk::raii::PhysicalDevices physicalDevices(m_instance);
    
    if (physicalDevices.empty()) {
        TE_LOG_ERROR("No physical devices found");
        return {};
    }
    
    TE_LOG_INFO("Found {} physical device(s)", physicalDevices.size());
    
    std::vector<std::shared_ptr<VulkanPhysicalDevice>> devices;
    devices.reserve(physicalDevices.size());
    
    for (auto& device : physicalDevices) {
        auto wrapped = std::make_shared<VulkanPhysicalDevice>(
            VulkanPhysicalDevice::PrivateTag{},
            weak_from_this(),
            std::move(device)
        );
        devices.push_back(std::move(wrapped));
    }
    
    return devices;
}

std::shared_ptr<VulkanSurface> VulkanContext::CreateSurface(Window& window) {
    VkSurfaceKHR surface;
    auto* glfwWindow = static_cast<GLFWwindow*>(window.GetNativeHandle());
    
    // GLFW 的创建 surface 函数仍然返回 VkResult，需要转换
    const VkResult result = glfwCreateWindowSurface(*m_instance, glfwWindow, nullptr, &surface);
    if (!VK_CHECK(static_cast<vk::Result>(result))) {
        TE_LOG_ERROR("Failed to create surface");
        return nullptr;
    }

    vk::raii::SurfaceKHR raiiSurface(m_instance, surface);
    
    TE_LOG_INFO("Surface created");
    
    return std::make_shared<VulkanSurface>(
        VulkanSurface::PrivateTag{},
        weak_from_this(),
        std::move(raiiSurface)
    );
}

} // namespace TE
