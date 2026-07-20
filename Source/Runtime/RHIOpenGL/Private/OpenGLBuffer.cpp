// ToyEngine RHIOpenGL Module
// OpenGL Buffer 实现

#include "OpenGLBuffer.h"
#include "Log/Log.h"

namespace TE {

static GLenum BufferUsageToGLTarget(RHIBufferUsage usage)
{
    if (HasAnyFlags(usage, RHIBufferUsage::Uniform))
    {
        return GL_UNIFORM_BUFFER;
    }
    if (HasAnyFlags(usage, RHIBufferUsage::Storage))
    {
        return GL_SHADER_STORAGE_BUFFER;
    }
    if (HasAnyFlags(usage, RHIBufferUsage::Index))
    {
        return GL_ELEMENT_ARRAY_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

static GLenum BufferUsageToGLHint(const RHIBufferDesc& desc)
{
    if (desc.memoryUsage != RHIMemoryUsage::GPUOnly ||
        HasAnyFlags(desc.usage, RHIBufferUsage::Uniform | RHIBufferUsage::Storage | RHIBufferUsage::Staging))
    {
        return GL_DYNAMIC_DRAW;
    }
    return GL_STATIC_DRAW;
}

OpenGLBuffer::OpenGLBuffer(const RHIBufferDesc& desc)
    : m_Size(desc.size)
    , m_Usage(desc.usage)
    , m_Target(BufferUsageToGLTarget(desc.usage))
{
    glGenBuffers(1, &m_BufferID);
    glBindBuffer(m_Target, m_BufferID);
    glBufferData(m_Target, static_cast<GLsizeiptr>(desc.size), desc.initialData, BufferUsageToGLHint(desc));
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
