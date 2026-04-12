// ToyEngine RHIOpenGL Module
// OpenGL Sampler 实现

#include "OpenGLSampler.h"

namespace TE {

namespace {

GLint ConvertFilter(RHITextureFilter filter)
{
    return (filter == RHITextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
}

GLint ConvertAddressMode(RHITextureAddressMode mode)
{
    return (mode == RHITextureAddressMode::ClampToEdge) ? GL_CLAMP_TO_EDGE : GL_REPEAT;
}

} // namespace

OpenGLSampler::OpenGLSampler(const RHISamplerDesc& desc)
{
    glGenSamplers(1, &m_SamplerID);
    if (m_SamplerID == 0)
    {
        return;
    }

    glSamplerParameteri(m_SamplerID, GL_TEXTURE_MIN_FILTER, ConvertFilter(desc.minFilter));
    glSamplerParameteri(m_SamplerID, GL_TEXTURE_MAG_FILTER, ConvertFilter(desc.magFilter));
    glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_S, ConvertAddressMode(desc.addressU));
    glSamplerParameteri(m_SamplerID, GL_TEXTURE_WRAP_T, ConvertAddressMode(desc.addressV));
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

