// ToyEngine RHIOpenGL Module
// OpenGL BindGroup 实现
// OpenGL 没有原生 DescriptorSet，此实现将绑定信息缓存下来，
// 在 SetBindGroup 时依次执行 glBindBufferRange / glActiveTexture+glBindTexture 等操作。

#pragma once

#include "RHIBindGroup.h"
#include "RHITypes.h"
#include <glad/glad.h>
#include <vector>

namespace TE {

class OpenGLBuffer;
class OpenGLTexture;
class OpenGLSampler;

/// 缓存单条绑定的 OpenGL 资源句柄
struct OpenGLBindEntry
{
    uint32_t       binding = 0;
    RHIBindingType type = RHIBindingType::UniformBuffer;

    // UniformBuffer
    GLuint   glBuffer = 0;
    uint64_t bufferOffset = 0;
    uint64_t bufferSize = 0;

    // Texture2D + Sampler
    GLuint glTexture = 0;
    GLuint glSampler = 0;
};

class OpenGLBindGroup final : public RHIBindGroup
{
public:
    explicit OpenGLBindGroup(const RHIBindGroupDesc& desc);
    ~OpenGLBindGroup() override = default;

    OpenGLBindGroup(const OpenGLBindGroup&) = delete;
    OpenGLBindGroup& operator=(const OpenGLBindGroup&) = delete;

    [[nodiscard]] bool IsValid() const override { return m_Valid; }
    [[nodiscard]] const std::vector<OpenGLBindEntry>& GetEntries() const { return m_Entries; }

private:
    std::vector<OpenGLBindEntry> m_Entries;
    bool m_Valid = false;
};

} // namespace TE
