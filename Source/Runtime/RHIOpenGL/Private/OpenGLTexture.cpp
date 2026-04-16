// ToyEngine RHIOpenGL Module
// OpenGL Texture 实现

#include "OpenGLTexture.h"

#include "Log/Log.h"

#include <cstring>
#include <vector>

namespace TE {

namespace {

bool ConvertFormat(RHIFormat format, bool srgb, GLint& outInternalFormat, GLenum& outFormat, GLenum& outType)
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
        outInternalFormat = srgb ? GL_SRGB8 : GL_RGB8;
        outFormat = GL_RGB;
        return true;
    case RHIFormat::RGBA8_UNorm:
        outInternalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        outFormat = GL_RGBA;
        return true;
    case RHIFormat::RGB8_sRGB:
        outInternalFormat = GL_SRGB8;
        outFormat = GL_RGB;
        return true;
    case RHIFormat::RGBA8_sRGB:
        outInternalFormat = GL_SRGB8_ALPHA8;
        outFormat = GL_RGBA;
        return true;
    // 深度 / 深度模板格式：用于 FBO 深度附件
    // OpenGLTexture 作为深度纹理时，不做 sRGB 转换，不参与像素行序翻转
    case RHIFormat::D16_UNorm:
        outInternalFormat = GL_DEPTH_COMPONENT16;
        outFormat = GL_DEPTH_COMPONENT;
        outType = GL_UNSIGNED_SHORT;
        return true;
    case RHIFormat::D24_UNorm_S8_UInt:
        outInternalFormat = GL_DEPTH24_STENCIL8;
        outFormat = GL_DEPTH_STENCIL;
        outType = GL_UNSIGNED_INT_24_8;
        return true;
    case RHIFormat::D32_Float:
        outInternalFormat = GL_DEPTH_COMPONENT32F;
        outFormat = GL_DEPTH_COMPONENT;
        outType = GL_FLOAT;
        return true;
    default:
        return false;
    }
}

/// 判断 RHIFormat 是否为深度 / 深度模板格式
bool IsDepthFormat(RHIFormat format)
{
    switch (format)
    {
    case RHIFormat::D16_UNorm:
    case RHIFormat::D24_UNorm_S8_UInt:
    case RHIFormat::D32_Float:
        return true;
    default:
        return false;
    }
}

/// 翻转像素行序（引擎约定纹理原点在左上角，OpenGL 原点在左下角）
void FlipImageVertically(const void* src, void* dst, uint32_t width, uint32_t height, uint32_t bytesPerPixel)
{
    const uint32_t rowBytes = width * bytesPerPixel;
    const auto* srcBytes = static_cast<const uint8_t*>(src);
    auto* dstBytes = static_cast<uint8_t*>(dst);

    for (uint32_t row = 0; row < height; ++row)
    {
        const uint32_t srcRow = height - 1 - row;
        std::memcpy(dstBytes + row * rowBytes, srcBytes + srcRow * rowBytes, rowBytes);
    }
}

uint32_t GetPixelByteSize(RHIFormat format)
{
    switch (format)
    {
    case RHIFormat::R8_UNorm:    return 1;
    case RHIFormat::RG8_UNorm:   return 2;
    case RHIFormat::RGB8_UNorm:  return 3;
    case RHIFormat::RGB8_sRGB:   return 3;
    case RHIFormat::RGBA8_UNorm: return 4;
    case RHIFormat::RGBA8_sRGB:  return 4;
    default: return 0;
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
    if (!ConvertFormat(desc.format, desc.srgb, internalFormat, format, type))
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

    const bool isDepth = IsDepthFormat(desc.format);

    // 引擎约定纹理原点为左上角，OpenGL 原点在左下角，需要翻转像素行序。
    // 对于 1x1、无初始数据、深度纹理（通常是 RT 附件而非 CPU 图像）跳过翻转。
    const void* uploadData = desc.initialData;
    std::vector<uint8_t> flippedBuffer;

    if (desc.initialData && desc.height > 1 && !isDepth)
    {
        uint32_t bpp = GetPixelByteSize(desc.format);
        if (bpp > 0)
        {
            flippedBuffer.resize(static_cast<size_t>(desc.width) * desc.height * bpp);
            FlipImageVertically(desc.initialData, flippedBuffer.data(), desc.width, desc.height, bpp);
            uploadData = flippedBuffer.data();
        }
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
                 uploadData);

    // 深度纹理作为 RT 附件时使用 CLAMP_TO_EDGE + NEAREST，且不生成 mipmap（FBO 渲染到 mip0）。
    // 彩色纹理沿用原有 REPEAT + LINEAR 行为，便于材质贴图采样。
    if (isDepth)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // 作为 RT 附件（无初始数据 + 不生成 mipmap）时用 LINEAR 基层采样，避免 MIP 过滤器要求完整 mip 链。
        const bool useMipFilter = desc.generateMips && desc.initialData != nullptr;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useMipFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (useMipFilter)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
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

