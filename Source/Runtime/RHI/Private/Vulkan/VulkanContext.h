// Vulkan Context - Instance 和基础初始化管理
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <string>
#include <vector>
#include <memory>

namespace TE {

class Window;
class VulkanPhysicalDevice;
class VulkanSurface;

/// Vulkan 上下文配置
struct VulkanContextConfig {
    std::string appName = "ToyEngine";
    uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
    uint32_t apiVersion = VK_API_VERSION_1_4;
    bool enableValidation = true;
};

/// Vulkan 上下文 - 管理 Instance 和 Debug Messenger
class VulkanContext : public std::enable_shared_from_this<VulkanContext> {
public:
    // Passkey 模式 - 防止外部直接构造
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanContext;  // 只有 VulkanContext 能构造 PrivateTag
    };

    // 工厂方法创建
    [[nodiscard]] static std::shared_ptr<VulkanContext> Create(const VulkanContextConfig& config = {});

    // 构造函数需要 PrivateTag，外部无法直接调用
    VulkanContext(PrivateTag, const VulkanContextConfig& config):m_config(config){}

    ~VulkanContext();

    // 禁用拷贝和移动
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    // 获取 Instance
    [[nodiscard]] const vk::raii::Instance& GetInstance() const { return m_instance; }
    [[nodiscard]] const vk::raii::Context& GetRaiiContext() const { return m_context; }
    
    // 检查验证层是否启用
    [[nodiscard]] bool IsValidationEnabled() const { return m_config.enableValidation; }

    // 枚举物理设备
    [[nodiscard]] std::vector<std::shared_ptr<VulkanPhysicalDevice>> EnumeratePhysicalDevices();

    // 创建 Surface（返回shared_ptr以确保生命周期安全）
    [[nodiscard]] std::shared_ptr<VulkanSurface> CreateSurface(Window& window);

private:
    bool Initialize();
    bool CreateInstance();
    void SetupDebugMessenger();
    
    static vk::DebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo();
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );
    
    // Vulkan-hpp 风格的回调函数
    static VkBool32 DebugCallbackCpp(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

private:
    VulkanContextConfig m_config;
    vk::raii::Context m_context;
    vk::raii::Instance m_instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger{nullptr};
};

} // namespace TE
