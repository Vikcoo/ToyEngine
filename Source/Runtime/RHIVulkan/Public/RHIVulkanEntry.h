// ToyEngine RHIVulkan Module
// Vulkan 后端公开装配入口

#pragma once

#include "RHIDevice.h"

#include <memory>

namespace TE {

/** 创建 Vulkan RHI Device；初始化失败时返回 nullptr。 */
[[nodiscard]] std::unique_ptr<RHIDevice> CreateRHIVulkanDevice(const RHIDeviceCreateDesc& desc);

} // namespace TE
