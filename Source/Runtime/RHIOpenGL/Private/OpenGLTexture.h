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

    [[nodiscard]] GLuint GetGLTextureID() const { return m_TextureID; }

private:
    GLuint    m_TextureID = 0;
    uint32_t  m_Width = 0;
    uint32_t  m_Height = 0;
    RHIFormat m_Format = RHIFormat::Undefined;
};

} // namespace TE

