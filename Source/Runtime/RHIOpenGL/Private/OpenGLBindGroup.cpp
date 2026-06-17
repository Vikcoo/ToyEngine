// ToyEngine RHIOpenGL Module
// OpenGL BindGroup 实现

#include "OpenGLBindGroup.h"
#include "OpenGLBindGroupLayout.h"
#include "OpenGLBuffer.h"
#include "OpenGLTexture.h"
#include "OpenGLSampler.h"
#include "Log/Log.h"

namespace TE {

OpenGLBindGroup::OpenGLBindGroup(const RHIBindGroupDesc& desc)
{
    if (!desc.layout || !desc.layout->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] BindGroup '{}' created without a valid layout", desc.debugName);
        return;
    }

    m_Layout = static_cast<const OpenGLBindGroupLayout*>(desc.layout);
    m_Entries.reserve(desc.entries.size());

    for (const auto& entry : desc.entries)
    {
        if (!ValidateEntryAgainstLayout(entry))
        {
            TE_LOG_ERROR("[RHIOpenGL] BindGroup '{}' entry binding {} does not match layout",
                         desc.debugName,
                         entry.binding);
            m_Entries.clear();
            return;
        }

        OpenGLBindEntry glEntry;
        glEntry.binding = entry.binding;
        glEntry.type = entry.type;

        switch (entry.type)
        {
        case RHIBindingType::UniformBuffer:
        {
            if (!entry.buffer) return;
            auto* glBuf = static_cast<OpenGLBuffer*>(entry.buffer);
            glEntry.glBuffer = glBuf->GetGLBufferID();
            glEntry.bufferOffset = entry.bufferOffset;
            glEntry.bufferSize = entry.bufferSize;
            break;
        }
        case RHIBindingType::Texture2D:
        case RHIBindingType::TextureCube:
        {
            if (!entry.texture) return;
            auto* glTex = static_cast<OpenGLTexture*>(entry.texture);
            glEntry.glTexture = glTex->GetGLTextureID();
            glEntry.glTextureTarget = glTex->GetGLTextureTarget();
            if (entry.sampler)
            {
                auto* glSmp = static_cast<OpenGLSampler*>(entry.sampler);
                glEntry.glSampler = glSmp->GetGLSamplerID();
            }
            break;
        }
        case RHIBindingType::Sampler:
        {
            if (!entry.sampler) return;
            auto* glSmp = static_cast<OpenGLSampler*>(entry.sampler);
            glEntry.glSampler = glSmp->GetGLSamplerID();
            break;
        }
        }

        m_Entries.push_back(glEntry);
    }

    m_Valid = true;
}

bool OpenGLBindGroup::ValidateEntryAgainstLayout(const RHIBindGroupEntry& entry) const
{
    if (!m_Layout)
    {
        return false;
    }

    for (const auto& layoutEntry : m_Layout->GetDesc().entries)
    {
        if (layoutEntry.binding == entry.binding)
        {
            return layoutEntry.type == entry.type;
        }
    }

    return false;
}

} // namespace TE
