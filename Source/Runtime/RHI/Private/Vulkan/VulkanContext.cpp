// Vulkan Context 实现
#include "VulkanContext.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"
#include "VulkanUtils.h"
#include "Window.h"
#include "Log/Log.h"

#include <GLFW/glfw3.h>

#include <utility>

namespace TE {

// ============================================================================
// 工厂方法和构造/析构
// ============================================================================

std::shared_ptr<VulkanContext> VulkanContext::Create(const VulkanContextConfig& config) {
    auto context = std::make_shared<VulkanContext>(PrivateTag{}, config);
    TE_LOG_INFO("Creating Vulkan Context, App: {}, Validation: {}", config.appName, config.enableValidation ? "Enabled" : "Disabled");
    if (!context->Initialize()) {
        TE_LOG_ERROR("Failed to initialize Vulkan context");
        return nullptr;
    }

    return context;
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

    // 验证所需扩展都被支持
    if (!CheckInstanceExtensionSupport(extensions)) {
        TE_LOG_ERROR("Required extensions not supported");
        return false;
    }
    
    // 启用验证层并检查是否支持验证层
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
              .setPEnabledExtensionNames(extensions)
              .setPEnabledLayerNames(layers);

    // Debugger
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (m_config.enableValidation && !layers.empty()) {
        debugCreateInfo = GetDebugMessengerCreateInfo();
        createInfo.setPNext(&debugCreateInfo);
    }
    
    // 创建 Instance
    m_instance = vk::raii::Instance(m_context, createInfo);
    if (m_instance == nullptr ){
        return false;
    }

    TE_LOG_INFO("Vulkan Instance created");
    return true;
}

// ============================================================================
// Debug Messenger
// ============================================================================

void VulkanContext::SetupDebugMessenger() {
    const vk::DebugUtilsMessengerCreateInfoEXT createInfo = GetDebugMessengerCreateInfo();

    m_debugMessenger = vk::raii::DebugUtilsMessengerEXT(m_instance, createInfo);
    if (m_debugMessenger == nullptr)
    {
        TE_LOG_WARN("Failed to setup Debug Messenger: {}");
    }
    TE_LOG_INFO("Debug Messenger setup complete");
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
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData)
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
    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData)
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
    vk::raii::PhysicalDevices physicalDevices(m_instance); // 枚举物理设备的另一种语法

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
        TE_LOG_DEBUG("Physical Device: {}", wrapped->GetDeviceName());
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
    if (raiiSurface == VK_NULL_HANDLE)
    {
        TE_LOG_ERROR("Failed to create surface vk::raii");
    }
    TE_LOG_INFO("Surface created");
    
    return std::make_shared<VulkanSurface>(
        VulkanSurface::PrivateTag{},
        weak_from_this(),
        std::move(raiiSurface)
    );
}

} // namespace TE
