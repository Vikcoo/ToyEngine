// ToyEngine Engine Module
// RHI Device 装配逻辑（外层模块负责选择具体后端）

#include "RHIDevice.h"
#include "Log/Log.h"

#if defined(TE_RHI_BACKEND_OPENGL)
    #include "RHIOpenGLEntry.h"
#endif

namespace TE {

std::unique_ptr<RHIDevice> RHIDevice::Create()
{
#if defined(TE_RHI_BACKEND_OPENGL)
    TE_LOG_INFO("[RHI] Creating RHIOpenGL Device");
    return CreateRHIOpenGLDevice();

// #elif defined(TE_RHI_BACKEND_VULKAN)
//     TE_LOG_INFO("[RHI] Creating Vulkan RHI Device");
//     return CreateVulkanRHIDevice();

// #elif defined(TE_RHI_BACKEND_D3D12)
//     TE_LOG_INFO("[RHI] Creating D3D12 RHI Device");
//     return CreateD3D12RHIDevice();

#else
    TE_LOG_ERROR("[RHI] No RHI backend enabled! Enable TE_RHI_OPENGL, TE_RHI_VULKAN, or TE_RHI_D3D12 in CMake.");
    return nullptr;
#endif
}

} // namespace TE
