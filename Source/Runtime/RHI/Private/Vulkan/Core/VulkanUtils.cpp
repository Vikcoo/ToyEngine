// Vulkan 工具函数实现
#include "VulkanUtils.h"
#include "Log/Log.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace TE {

std::string VkResultToString(const vk::Result result) {
    // 使用 vulkan-hpp 的 to_string 功能
    return vk::to_string(result);
}

bool CheckVkResult(const vk::Result result, const char* file, int line, const char* function) {
    if (result != vk::Result::eSuccess) {
        TE_LOG_ERROR("Vulkan Error: {} at {}:{} in {}", 
                     VkResultToString(result), file, line, function);
        return false;
    }
    return true;
}

bool CheckValidationLayerSupport(const std::vector<const char*>& layers) {
    // 使用 vulkan-hpp 风格
    const auto available = vk::enumerateInstanceLayerProperties();
    
    for (const char* layerName : layers) {
        bool found = false;
        for (const auto& props : available) {
            if (strcmp(layerName, props.layerName) == 0) {
                found = true;
                TE_LOG_DEBUG("Validation layer found: {}", layerName);
                break;
            }
        }
        if (!found) {
            TE_LOG_ERROR("Validation layer not found: {}", layerName);
            return false;
        }
    }
    
    return true;
}

bool CheckInstanceExtensionSupport(const std::vector<const char*>& extensions) {
    const auto available = vk::enumerateInstanceExtensionProperties();
    
    for (const char* ext : extensions) {
        bool found = false;
        for (const auto& props : available) {
            if (strcmp(ext, props.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            TE_LOG_ERROR("Instance extension not found: {}", ext);
            return false;
        }
    }
    
    return true;
}

bool CheckDeviceExtensionSupport(const vk::PhysicalDevice& device, const std::vector<const char*>& extensions) {
    // 使用 vulkan-hpp 风格
    const auto available = device.enumerateDeviceExtensionProperties();
    
    for (const char* ext : extensions) {
        bool found = false;
        for (const auto& props : available) {
            if (strcmp(ext, props.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    
    return true;
}

std::vector<const char*> GetRequiredInstanceExtensions(const bool enableValidation) {
    uint32_t glfwExtCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtCount);
    
    if (enableValidation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

} // namespace TE
