// ToyEngine OpenGLRHI Module
// OpenGL Buffer 实现

#include "OpenGLBuffer.h"
#include "Log/Log.h"

namespace TE {

static GLenum BufferUsageToGLTarget(RHIBufferUsage usage)
{
    switch (usage)
    {
        case RHIBufferUsage::Vertex:  return GL_ARRAY_BUFFER;
        case RHIBufferUsage::Index:   return GL_ELEMENT_ARRAY_BUFFER;
        case RHIBufferUsage::Uniform: return GL_UNIFORM_BUFFER;
        default:                      return GL_ARRAY_BUFFER;
    }
}

OpenGLBuffer::OpenGLBuffer(const RHIBufferDesc& desc)
    : m_Size(desc.size)
    , m_Usage(desc.usage)
    , m_Target(BufferUsageToGLTarget(desc.usage))
{
    glGenBuffers(1, &m_BufferID);
    glBindBuffer(m_Target, m_BufferID);
    glBufferData(m_Target, static_cast<GLsizeiptr>(desc.size), desc.initialData, GL_STATIC_DRAW);
    glBindBuffer(m_Target, 0);

    if (!desc.debugName.empty())
    {
        TE_LOG_DEBUG("[OpenGLRHI] Created Buffer '{}': ID={}, size={} bytes", desc.debugName, m_BufferID, m_Size);
    }
}

OpenGLBuffer::~OpenGLBuffer()
{
    if (m_BufferID != 0)
    {
        glDeleteBuffers(1, &m_BufferID);
        m_BufferID = 0;
    }
}

} // namespace TE
