// ToyEngine RHIOpenGL Module
// OpenGL Buffer 实现 - 封装 OpenGL VBO/IBO/UBO

#pragma once

#include "RHIBuffer.h"
#include <glad/glad.h>

namespace TE {

class OpenGLBuffer final : public RHIBuffer
{
public:
    OpenGLBuffer(const RHIBufferDesc& desc);
    ~OpenGLBuffer() override;

    // 禁止拷贝
    OpenGLBuffer(const OpenGLBuffer&) = delete;
    OpenGLBuffer& operator=(const OpenGLBuffer&) = delete;

    [[nodiscard]] uint64_t GetSize() const override { return m_Size; }
    [[nodiscard]] RHIBufferUsage GetUsage() const override { return m_Usage; }

    /// 获取 OpenGL 缓冲区 ID
    [[nodiscard]] GLuint GetGLBufferID() const { return m_BufferID; }

    /// 获取 OpenGL 缓冲区目标（GL_ARRAY_BUFFER / GL_ELEMENT_ARRAY_BUFFER 等）
    [[nodiscard]] GLenum GetGLTarget() const { return m_Target; }

private:
    GLuint          m_BufferID = 0;
    GLenum          m_Target = GL_ARRAY_BUFFER;
    uint64_t        m_Size = 0;
    RHIBufferUsage  m_Usage = RHIBufferUsage::Vertex;
};

} // namespace TE
