// ToyEngine RHIOpenGL Module
// OpenGL Sampler 实现

#include "OpenGLSampler.h"

#include <algorithm>
#include <cstring>

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace TE {

namespace {

GLint ConvertMinFilter(RHITextureFilter filter)
{
    switch (filter)
    {
    case RHITextureFilter::Nearest:
        return GL_NEAREST;
    case RHITextureFilter::LinearMipmapLinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    case RHITextureFilter::Linear:
    default:
        return GL_LINEAR;
    }
}

GLint ConvertMagFilter(RHITextureFilter filter)
{
    return (filter == RHITextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
}

GLint ConvertAddressMode(RHITextureAddressMode mode)
{
    return (mode == RHITextureAddressMode::ClampToEdge) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
}

bool SupportsAnisotropicFiltering()
{
    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    for (GLint i = 0; i < extensionCount; ++i)
    {
        const auto* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        if (extension && std::strcmp(extension, "GL_EXT_texture_filter_anisotropic") == 0)
        {
            return true;
        }
    }
    return false;
}

} // namespace

OpenGLSampler::OpenGLSampler(const RHISamplerDesc& desc)
{
    glGenSamplers(1, &m_SamplerID);
    if (m_SamplerID == 0)
    {
        return;
    }

    glSamplerParameteri(m_SamplerID, GL_TEXTURE_MIN_FILTER, ConvertMinFilter(desc.minFilter));
    glSamplerParameteri(m_SamplerID, GL_TEXTURE_MAG_FILTER, ConvertMagFilter(desc.magFilter));
    glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_S, ConvertAddressMode(desc.addressU));
    glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_T, ConvertAddressMode(desc.addressV));
    glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_R, ConvertAddressMode(desc.addressW));

    if (desc.enableAnisotropy && desc.maxAnisotropy > 1.0f && SupportsAnisotropicFiltering())
    {
        GLfloat maxSupportedAnisotropy = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxSupportedAnisotropy);
        const GLfloat requestedAnisotropy = std::clamp(desc.maxAnisotropy, 1.0f, maxSupportedAnisotropy);
        glSamplerParameterf(m_SamplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, requestedAnisotropy);
    }
}

OpenGLSampler::~OpenGLSampler()
{
    if (m_SamplerID != 0)
    {
        glDeleteSamplers(1, &m_SamplerID);
        m_SamplerID = 0;
    }
}

} // namespace TE
