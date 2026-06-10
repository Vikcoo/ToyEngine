// ToyEngine RHIOpenGL Module
// OpenGL PipelineLayout 实现

#include "OpenGLPipelineLayout.h"

#include "RHIBindGroup.h"
#include "Log/Log.h"

namespace TE {

OpenGLPipelineLayout::OpenGLPipelineLayout(const RHIPipelineLayoutDesc& desc)
    : m_Desc(desc)
{
    for (size_t i = 0; i < m_Desc.bindGroupLayouts.size(); ++i)
    {
        const auto& bindGroupLayout = m_Desc.bindGroupLayouts[i];
        if (!bindGroupLayout.layout || !bindGroupLayout.layout->IsValid())
        {
            TE_LOG_ERROR("[RHIOpenGL] PipelineLayout '{}' contains invalid BindGroupLayout", m_Desc.debugName);
            return;
        }

        for (size_t j = i + 1; j < m_Desc.bindGroupLayouts.size(); ++j)
        {
            if (bindGroupLayout.groupIndex == m_Desc.bindGroupLayouts[j].groupIndex)
            {
                TE_LOG_ERROR("[RHIOpenGL] PipelineLayout '{}' has duplicated group index {}",
                             m_Desc.debugName,
                             bindGroupLayout.groupIndex);
                return;
            }
        }
    }

    m_Valid = true;
}

} // namespace TE
