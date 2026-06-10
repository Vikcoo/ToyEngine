// ToyEngine RHIOpenGL Module
// OpenGL BindGroupLayout 实现

#include "OpenGLBindGroupLayout.h"

#include "Log/Log.h"

namespace TE {

OpenGLBindGroupLayout::OpenGLBindGroupLayout(const RHIBindGroupLayoutDesc& desc)
    : m_Desc(desc)
{
    for (size_t i = 0; i < m_Desc.entries.size(); ++i)
    {
        for (size_t j = i + 1; j < m_Desc.entries.size(); ++j)
        {
            if (m_Desc.entries[i].binding == m_Desc.entries[j].binding)
            {
                TE_LOG_ERROR("[RHIOpenGL] BindGroupLayout '{}' has duplicated binding {}",
                             m_Desc.debugName,
                             m_Desc.entries[i].binding);
                return;
            }
        }
    }

    m_Valid = true;
}

} // namespace TE
