// Vulkan 工具函数
#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <string>
#include <vector>

namespace TE {

// vk::Result 转字符串
std::string VkResultToString(vk::Result result);

// 检查 vk::Result
bool CheckVkResult(vk::Result result, const char* file, int line, const char* function);

// 验证层支持检查
bool CheckValidationLayerSupport(const std::vector<const char*>& layers);

// 扩展支持检查
bool CheckInstanceExtensionSupport(const std::vector<const char*>& extensions);
bool CheckDeviceExtensionSupport(const vk::PhysicalDevice& device, const std::vector<const char*>& extensions);

// 获取所需扩展
std::vector<const char*> GetRequiredInstanceExtensions(bool enableValidation);

// 检查宏
#define VK_CHECK(result) TE::CheckVkResult(result, __FILE__, __LINE__, __FUNCTION__)

} // namespace TE
