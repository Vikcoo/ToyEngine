// ToyEngine RHIOpenGL Module
// OpenGL Texture 实现

#include "OpenGLTexture.h"

#include "Log/Log.h"

namespace TE {

namespace {

bool ConvertFormat(RHIFormat format, GLint& outInternalFormat, GLenum& outFormat, GLenum& outType)
{
    outType = GL_UNSIGNED_BYTE;
    switch (format)
    {
    case RHIFormat::R8_UNorm:
        outInternalFormat = GL_R8;
        outFormat = GL_RED;
        return true;
    case RHIFormat::RG8_UNorm:
        outInternalFormat = GL_RG8;
        outFormat = GL_RG;
        return true;
    case RHIFormat::RGB8_UNorm:
        outInternalFormat = GL_RGB8;
        outFormat = GL_RGB;
        return true;
    case RHIFormat::RGBA8_UNorm:
        outInternalFormat = GL_RGBA8;
        outFormat = GL_RGBA;
        return true;
    default:
        return false;
    }
}

} // namespace

OpenGLTexture::OpenGLTexture(const RHITextureDesc& desc)
    : m_Width(desc.width)
    , m_Height(desc.height)
    , m_Format(desc.format)
{
    if (desc.width == 0 || desc.height == 0)
    {
        TE_LOG_ERROR("[RHIOpenGL] OpenGLTexture create failed: invalid size {}x{}", desc.width, desc.height);
        return;
    }

    GLint internalFormat = GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    if (!ConvertFormat(desc.format, internalFormat, format, type))
    {
        TE_LOG_ERROR("[RHIOpenGL] OpenGLTexture create failed: unsupported format {}", static_cast<int>(desc.format));
        return;
    }

    glGenTextures(1, &m_TextureID);
    if (m_TextureID == 0)
    {
        TE_LOG_ERROR("[RHIOpenGL] OpenGLTexture create failed: glGenTextures returned 0");
        return;
    }

    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internalFormat,
                 static_cast<GLsizei>(desc.width),
                 static_cast<GLsizei>(desc.height),
                 0,
                 format,
                 type,
                 desc.initialData);

    // 默认参数，若绑定 Sampler 对象会覆盖采样状态。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc.generateMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (desc.generateMips)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

OpenGLTexture::~OpenGLTexture()
{
    if (m_TextureID != 0)
    {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
}

} // namespace TE

