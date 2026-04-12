// ToyEngine RHIOpenGL Module
// OpenGL Sampler 实现 - 封装 OpenGL Sampler 对象

#pragma once

#include "RHISampler.h"
#include <glad/glad.h>

namespace TE {

class OpenGLSampler final : public RHISampler
{
public:
    explicit OpenGLSampler(const RHISamplerDesc& desc);
    ~OpenGLSampler() override;

    OpenGLSampler(const OpenGLSampler&) = delete;
    OpenGLSampler& operator=(const OpenGLSampler&) = delete;

    [[nodiscard]] bool IsValid() const override { return m_SamplerID != 0; }
    [[nodiscard]] GLuint GetGLSamplerID() const { return m_SamplerID; }

private:
    GLuint m_SamplerID = 0;
};

} // namespace TE

