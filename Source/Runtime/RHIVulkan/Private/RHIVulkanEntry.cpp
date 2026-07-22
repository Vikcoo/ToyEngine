// ToyEngine RHIVulkan Module
// Vulkan 后端公开装配入口实现

#include "RHIVulkanEntry.h"

#include "VulkanDevice.h"
#include "Log/Log.h"

namespace TE {

std::unique_ptr<RHIDevice> CreateRHIVulkanDevice(const RHIDeviceCreateDesc& desc)
{
    auto device = std::make_unique<VulkanDevice>(desc);
    if (!device->IsValid())
    {
        TE_LOG_ERROR("[RHIVulkan] Device initialization failed");
        return nullptr;
    }
    return device;
}

} // namespace TE
