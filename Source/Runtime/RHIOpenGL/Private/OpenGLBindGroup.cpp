// ToyEngine RHIOpenGL Module
// OpenGL BindGroup 实现

#include "OpenGLBindGroup.h"
#include "OpenGLBuffer.h"
#include "OpenGLTexture.h"
#include "OpenGLSampler.h"

namespace TE {

OpenGLBindGroup::OpenGLBindGroup(const RHIBindGroupDesc& desc)
{
    m_Entries.reserve(desc.entries.size());

    for (const auto& entry : desc.entries)
    {
        OpenGLBindEntry glEntry;
        glEntry.binding = entry.binding;
        glEntry.type = entry.type;

        switch (entry.type)
        {
        case RHIBindingType::UniformBuffer:
        {
            if (!entry.buffer) continue;
            auto* glBuf = static_cast<OpenGLBuffer*>(entry.buffer);
            glEntry.glBuffer = glBuf->GetGLBufferID();
            glEntry.bufferOffset = entry.bufferOffset;
            glEntry.bufferSize = entry.bufferSize;
            break;
        }
        case RHIBindingType::Texture2D:
        {
            if (!entry.texture) continue;
            auto* glTex = static_cast<OpenGLTexture*>(entry.texture);
            glEntry.glTexture = glTex->GetGLTextureID();
            if (entry.sampler)
            {
                auto* glSmp = static_cast<OpenGLSampler*>(entry.sampler);
                glEntry.glSampler = glSmp->GetGLSamplerID();
            }
            break;
        }
        case RHIBindingType::Sampler:
        {
            if (!entry.sampler) continue;
            auto* glSmp = static_cast<OpenGLSampler*>(entry.sampler);
            glEntry.glSampler = glSmp->GetGLSamplerID();
            break;
        }
        }

        m_Entries.push_back(glEntry);
    }

    m_Valid = true;
}

} // namespace TE
