// ToyEngine RHIOpenGL Module
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

static GLenum BufferUsageToGLHint(RHIBufferUsage usage)
{
    switch (usage)
    {
        case RHIBufferUsage::Uniform:
        case RHIBufferUsage::Storage:
        case RHIBufferUsage::Staging:
            return GL_DYNAMIC_DRAW;
        case RHIBufferUsage::Vertex:
        case RHIBufferUsage::Index:
        default:
            return GL_STATIC_DRAW;
    }
}

OpenGLBuffer::OpenGLBuffer(const RHIBufferDesc& desc)
    : m_Size(desc.size)
    , m_Usage(desc.usage)
    , m_Target(BufferUsageToGLTarget(desc.usage))
{
    glGenBuffers(1, &m_BufferID);
    glBindBuffer(m_Target, m_BufferID);
    glBufferData(m_Target, static_cast<GLsizeiptr>(desc.size), desc.initialData, BufferUsageToGLHint(desc.usage));
    glBindBuffer(m_Target, 0);

    if (!desc.debugName.empty())
    {
        TE_LOG_DEBUG("[RHIOpenGL] Created Buffer '{}': ID={}, size={} bytes", desc.debugName, m_BufferID, m_Size);
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

bool OpenGLBuffer::UpdateData(const void* data, uint64_t size, uint64_t offset)
{
    if (m_BufferID == 0 || !data || size == 0 || offset + size > m_Size)
    {
        return false;
    }

    glBindBuffer(m_Target, m_BufferID);
    glBufferSubData(m_Target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    glBindBuffer(m_Target, 0);
    return true;
}

} // namespace TE
