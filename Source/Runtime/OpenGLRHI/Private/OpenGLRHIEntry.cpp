// ToyEngine OpenGLRHI Module
// OpenGL 后端装配入口实现

#include "OpenGLRHIEntry.h"

#include "OpenGLDevice.h"

namespace TE {

std::unique_ptr<RHIDevice> CreateOpenGLRHIDevice()
{
    return std::make_unique<OpenGLDevice>();
}

} // namespace TE
