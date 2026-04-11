// ToyEngine RHIOpenGL Module
// RHIOpenGL 后端装配入口实现

#include "RHIOpenGLEntry.h"

#include "OpenGLDevice.h"

namespace TE {

std::unique_ptr<RHIDevice> CreateRHIOpenGLDevice()
{
    return std::make_unique<OpenGLDevice>();
}

} // namespace TE
