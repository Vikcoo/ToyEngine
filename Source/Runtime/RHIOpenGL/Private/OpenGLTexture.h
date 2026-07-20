// ToyEngine RHIOpenGL Module
// OpenGL Texture 实现 - 封装 OpenGL 2D 纹理对象

#pragma once

#include "RHITexture.h"
#include <glad/glad.h>

namespace TE {

class OpenGLTexture final : public RHITexture
{
public:
    explicit OpenGLTexture(const RHITextureDesc& desc);
    ~OpenGLTexture() override;

    OpenGLTexture(const OpenGLTexture&) = delete;
    OpenGLTexture& operator=(const OpenGLTexture&) = delete;

    [[nodiscard]] bool IsValid() const override { return m_TextureID != 0; }
    [[nodiscard]] uint32_t GetWidth() const override { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const override { return m_Height; }
    [[nodiscard]] RHIFormat GetFormat() const override { return m_Format; }
    [[nodiscard]] RHITextureUsage GetUsage() const override { return m_Usage; }
    [[nodiscard]] RHISampleCount GetSampleCount() const override { return m_SampleCount; }

    [[nodiscard]] GLuint GetGLTextureID() const { return m_TextureID; }
    [[nodiscard]] GLenum GetGLTextureTarget() const { return m_TextureTarget; }
    [[nodiscard]] RHIResourceState GetCurrentState() const { return m_CurrentState; }
    void SetCurrentState(RHIResourceState state) { m_CurrentState = state; }

private:
    GLuint    m_TextureID = 0;
    GLenum    m_TextureTarget = GL_TEXTURE_2D;
    uint32_t  m_Width = 0;
    uint32_t  m_Height = 0;
    RHIFormat m_Format = RHIFormat::Undefined;
    RHITextureUsage m_Usage = RHITextureUsage::None;
    RHISampleCount m_SampleCount = RHISampleCount::Count1;
    RHIResourceState m_CurrentState = RHIResourceState::Undefined;
};

} // namespace TE
