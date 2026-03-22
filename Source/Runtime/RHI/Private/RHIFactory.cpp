// ToyEngine RHI Module
// RHI 工厂方法实现 - 根据编译选项创建对应后端的 Device

#include "RHIDevice.h"
#include "Log/Log.h"

// 根据编译选项引入对应后端头文件
#if defined(TE_RHI_BACKEND_OPENGL)
    #include "OpenGLDevice.h"
#endif

// #if defined(TE_RHI_BACKEND_VULKAN)
//     #include "VulkanDevice.h"
// #endif

// #if defined(TE_RHI_BACKEND_D3D12)
//     #include "D3D12Device.h"
// #endif

namespace TE {

std::unique_ptr<RHIDevice> RHIDevice::Create()
{
#if defined(TE_RHI_BACKEND_OPENGL)
    TE_LOG_INFO("[RHI] Creating OpenGL RHI Device");
    return std::make_unique<OpenGLDevice>();

// #elif defined(TE_RHI_BACKEND_VULKAN)
//     TE_LOG_INFO("[RHI] Creating Vulkan RHI Device");
//     return std::make_unique<VulkanDevice>();

// #elif defined(TE_RHI_BACKEND_D3D12)
//     TE_LOG_INFO("[RHI] Creating D3D12 RHI Device");
//     return std::make_unique<D3D12Device>();

#else
    TE_LOG_ERROR("[RHI] No RHI backend enabled! Enable TE_RHI_OPENGL, TE_RHI_VULKAN, or TE_RHI_D3D12 in CMake.");
    return nullptr;
#endif
}

} // namespace TE
