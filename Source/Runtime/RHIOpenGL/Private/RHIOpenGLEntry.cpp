// ToyEngine RHIOpenGL Module
// RHIOpenGL 后端装配入口实现

#include "RHIOpenGLEntry.h"

#include "OpenGLDevice.h"

namespace TE {

std::unique_ptr<RHIDevice> CreateRHIOpenGLDevice(const RHIDeviceCreateDesc& desc)
{
    return std::make_unique<OpenGLDevice>(desc);
}

} // namespace TE
