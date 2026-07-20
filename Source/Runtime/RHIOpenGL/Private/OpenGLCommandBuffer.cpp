// ToyEngine RHIOpenGL Module
// OpenGL CommandBuffer 实现 - Immediate mode 执行

#include "OpenGLCommandBuffer.h"
#include "OpenGLPipeline.h"
#include "OpenGLBuffer.h"
#include "OpenGLBindGroup.h"
#include "OpenGLRenderTarget.h"
#include "Log/Log.h"

#include <array>

namespace TE {

namespace {

GLenum BlendFactorToGL(const RHIBlendFactor factor)
{
    switch (factor)
    {
    case RHIBlendFactor::Zero: return GL_ZERO;
    case RHIBlendFactor::One: return GL_ONE;
    case RHIBlendFactor::SourceColor: return GL_SRC_COLOR;
    case RHIBlendFactor::OneMinusSourceColor: return GL_ONE_MINUS_SRC_COLOR;
    case RHIBlendFactor::SourceAlpha: return GL_SRC_ALPHA;
    case RHIBlendFactor::OneMinusSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
    case RHIBlendFactor::DestinationColor: return GL_DST_COLOR;
    case RHIBlendFactor::OneMinusDestinationColor: return GL_ONE_MINUS_DST_COLOR;
    case RHIBlendFactor::DestinationAlpha: return GL_DST_ALPHA;
    case RHIBlendFactor::OneMinusDestinationAlpha: return GL_ONE_MINUS_DST_ALPHA;
    }
    return GL_ONE;
}

GLenum BlendOpToGL(const RHIBlendOp op)
{
    switch (op)
    {
    case RHIBlendOp::Add: return GL_FUNC_ADD;
    case RHIBlendOp::Subtract: return GL_FUNC_SUBTRACT;
    case RHIBlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
    case RHIBlendOp::Min: return GL_MIN;
    case RHIBlendOp::Max: return GL_MAX;
    }
    return GL_FUNC_ADD;
}

} // namespace

void OpenGLCommandBuffer::Begin()
{
    m_IsRecording = true;
    m_BoundPipeline = nullptr;
}

void OpenGLCommandBuffer::BeginRenderPass(const RHIRenderPassBeginInfo& info)
{
    // 绑定渲染目标：自定义 FBO 或默认帧缓冲（0）
    uint32_t colorAttachmentCount = 0;
    if (info.renderTarget != nullptr)
    {
        const auto* glRT = dynamic_cast<OpenGLRenderTarget*>(info.renderTarget);
        m_CurrentFBO = glRT->GetGLFramebufferID();
        colorAttachmentCount = glRT->GetColorAttachmentCount();

        glBindFramebuffer(GL_FRAMEBUFFER, m_CurrentFBO);

        // 为 MRT 启用所有 color attachment 的绘制槽位
        if (colorAttachmentCount > 0)
        {
            // 最大同时支持 8 个 color attachment，和 OpenGL 最小实现保证一致
            std::array<GLenum, 8> drawBuffers{};
            const uint32_t n = std::min<uint32_t>(colorAttachmentCount, static_cast<uint32_t>(drawBuffers.size()));
            for (uint32_t i = 0; i < n; ++i)
            {
                drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
            }
            glDrawBuffers(static_cast<GLsizei>(n), drawBuffers.data());
        }
        else
        {
            // 仅深度目标（如 ShadowMap），显式禁用颜色写入
            glDrawBuffer(GL_NONE);
        }
    }
    else
    {
        m_CurrentFBO = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);
    }

    // 设置视口
    if (info.viewport.width > 0 && info.viewport.height > 0)
    {
        glViewport(
            static_cast<GLint>(info.viewport.x),
            static_cast<GLint>(info.viewport.y),
            static_cast<GLsizei>(info.viewport.width),
            static_cast<GLsizei>(info.viewport.height)
        );
    }

    GLbitfield clearMask = 0;
    if (info.colorLoadOp == RHIRenderPassBeginInfo::LoadOp::Clear &&
        (info.renderTarget == nullptr || colorAttachmentCount > 0))
    {
        glClearColor(info.clearColor[0], info.clearColor[1], info.clearColor[2], info.clearColor[3]);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }

    if (info.depthLoadOp == RHIRenderPassBeginInfo::LoadOp::Clear)
    {
        // glClear 受 depth write mask 影响，清除前必须显式开启。
        glDepthMask(GL_TRUE);
        glClearDepth(info.clearDepth);
        clearMask |= GL_DEPTH_BUFFER_BIT;

        if (info.renderTarget && info.renderTarget->GetDepthStencilAttachment() &&
            info.renderTarget->GetDepthStencilAttachment()->GetFormat() == RHIFormat::D24_UNorm_S8_UInt)
        {
            glClearStencil(static_cast<GLint>(info.clearStencil));
            clearMask |= GL_STENCIL_BUFFER_BIT;
        }
    }

    if (clearMask != 0)
    {
        glClear(clearMask);
    }
}

void OpenGLCommandBuffer::EndRenderPass()
{
    // 解绑自定义 FBO，回到默认帧缓冲，避免状态泄漏到下一个 Pass 或 SwapBuffers。
    // Vulkan/D3D12 后端在此处执行 vkCmdEndRenderPass / OMSetRenderTargets(null)。
    if (m_CurrentFBO != 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_CurrentFBO = 0;
    }
}

void OpenGLCommandBuffer::BindPipeline(RHIPipeline* pipeline)
{
    if (!pipeline)
    {
        TE_LOG_WARN("[RHIOpenGL] BindPipeline called with null pipeline");
        return;
    }

    m_BoundPipeline = static_cast<OpenGLPipeline*>(pipeline);

    // 绑定 Program
    glUseProgram(m_BoundPipeline->GetGLProgram());

    // 绑定 VAO
    glBindVertexArray(m_BoundPipeline->GetVAO());

    // 应用管线光栅化/深度状态
    ApplyPipelineState();
}

void OpenGLCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32_t binding, uint64_t offset)
{
    if (!buffer)
    {
        TE_LOG_WARN("[RHIOpenGL] BindVertexBuffer called with null buffer");
        return;
    }

    if (!m_BoundPipeline)
    {
        TE_LOG_WARN("[RHIOpenGL] BindVertexBuffer called without bound pipeline");
        return;
    }

    auto* glBuffer = static_cast<OpenGLBuffer*>(buffer);

    // VAO 应该已经在 BindPipeline 时绑定了
    // 在 VAO 绑定状态下绑定 VBO，然后设置顶点属性指针
    // 这样 VAO 才能正确记住 VBO 关联
    glBindBuffer(GL_ARRAY_BUFFER, glBuffer->GetGLBufferID());

    // 重新配置顶点属性指针（在当前 VAO + VBO 绑定状态下）
    const auto& vertexInput = m_BoundPipeline->GetVertexInputDesc();
    for (const auto& attr : vertexInput.attributes)
    {
        // 查找对应 binding 的 stride
        uint32_t stride = 0;
        for (const auto& bind : vertexInput.bindings)
        {
            if (bind.binding == binding)
            {
                stride = bind.stride;
                break;
            }
        }

        GLenum glType = GL_FLOAT;
        GLint components = 0;

        switch (attr.format)
        {
            case RHIFormat::Float:  glType = GL_FLOAT; components = 1; break;
            case RHIFormat::Float2: glType = GL_FLOAT; components = 2; break;
            case RHIFormat::Float3: glType = GL_FLOAT; components = 3; break;
            case RHIFormat::Float4: glType = GL_FLOAT; components = 4; break;
            case RHIFormat::Int:    glType = GL_INT;   components = 1; break;
            case RHIFormat::Int2:   glType = GL_INT;   components = 2; break;
            case RHIFormat::Int3:   glType = GL_INT;   components = 3; break;
            case RHIFormat::Int4:   glType = GL_INT;   components = 4; break;
            case RHIFormat::UInt:   glType = GL_UNSIGNED_INT; components = 1; break;
            case RHIFormat::UInt2:  glType = GL_UNSIGNED_INT; components = 2; break;
            case RHIFormat::UInt3:  glType = GL_UNSIGNED_INT; components = 3; break;
            case RHIFormat::UInt4:  glType = GL_UNSIGNED_INT; components = 4; break;
            default:                glType = GL_FLOAT; components = 4; break;
        }

        glEnableVertexAttribArray(attr.location);
        glVertexAttribPointer(
            attr.location,
            components,
            glType,
            GL_FALSE,
            static_cast<GLsizei>(stride),
            reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.offset + offset))
        );
    }
}

void OpenGLCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, RHIIndexType indexType, uint64_t offset)
{
    if (!buffer)
    {
        TE_LOG_WARN("[RHIOpenGL] BindIndexBuffer called with null buffer");
        return;
    }

    auto* glBuffer = static_cast<OpenGLBuffer*>(buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glBuffer->GetGLBufferID());
}

void OpenGLCommandBuffer::SetViewport(const RHIViewport& viewport)
{
    glViewport(
        static_cast<GLint>(viewport.x),
        static_cast<GLint>(viewport.y),
        static_cast<GLsizei>(viewport.width),
        static_cast<GLsizei>(viewport.height)
    );
}

void OpenGLCommandBuffer::SetScissor(const RHIScissorRect& scissor)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor.x, scissor.y,
              static_cast<GLsizei>(scissor.width),
              static_cast<GLsizei>(scissor.height));
}

void OpenGLCommandBuffer::TransitionTexture(const RHITextureBarrier& barrier)
{
    if (!barrier.texture || barrier.before == barrier.after)
    {
        return;
    }

    auto* texture = static_cast<OpenGLTexture*>(barrier.texture);
    if (texture->GetCurrentState() != barrier.before)
    {
        TE_LOG_WARN("[RHIOpenGL] Texture transition state mismatch: expected {}, actual {}, target {}",
                    static_cast<uint32_t>(barrier.before),
                    static_cast<uint32_t>(texture->GetCurrentState()),
                    static_cast<uint32_t>(barrier.after));
    }

    GLbitfield barrierBits = 0;
    if (barrier.before == RHIResourceState::RenderTarget || barrier.before == RHIResourceState::DepthWrite)
    {
        barrierBits |= GL_FRAMEBUFFER_BARRIER_BIT;
    }
    if (barrier.before == RHIResourceState::CopyDestination)
    {
        barrierBits |= GL_TEXTURE_UPDATE_BARRIER_BIT;
    }
    if (barrier.after == RHIResourceState::ShaderResource)
    {
        barrierBits |= GL_TEXTURE_FETCH_BARRIER_BIT;
    }
    if (barrierBits != 0)
    {
        glMemoryBarrier(barrierBits);
    }

    texture->SetCurrentState(barrier.after);
}

void OpenGLCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex,
                                uint32_t instanceCount, uint32_t firstInstance)
{
    if (!m_BoundPipeline)
    {
        TE_LOG_ERROR("[RHIOpenGL] Draw called without bound pipeline");
        return;
    }

    GLenum topology = m_BoundPipeline->GetGLPrimitiveTopology();

    if (instanceCount > 1)
    {
        glDrawArraysInstanced(topology,
                              static_cast<GLint>(firstVertex),
                              static_cast<GLsizei>(vertexCount),
                              static_cast<GLsizei>(instanceCount));
    }
    else
    {
        glDrawArrays(topology,
                     static_cast<GLint>(firstVertex),
                     static_cast<GLsizei>(vertexCount));
    }
}

void OpenGLCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                                       int32_t vertexOffset,
                                       uint32_t instanceCount, uint32_t firstInstance)
{
    if (!m_BoundPipeline)
    {
        TE_LOG_ERROR("[RHIOpenGL] DrawIndexed called without bound pipeline");
        return;
    }

    GLenum topology = m_BoundPipeline->GetGLPrimitiveTopology();

    // 计算索引偏移（firstIndex * sizeof(uint32_t)）
    const void* indexOffset = reinterpret_cast<const void*>(
        static_cast<uintptr_t>(firstIndex * sizeof(uint32_t))
    );

    if (instanceCount > 1)
    {
        glDrawElementsInstanced(topology,
                                static_cast<GLsizei>(indexCount),
                                GL_UNSIGNED_INT,
                                indexOffset,
                                static_cast<GLsizei>(instanceCount));
    }
    else
    {
        glDrawElements(topology,
                       static_cast<GLsizei>(indexCount),
                       GL_UNSIGNED_INT,
                       indexOffset);
    }
}

void OpenGLCommandBuffer::SetBindGroup(const uint32_t groupIndex,
                                       RHIBindGroup* bindGroup,
                                       const std::span<const uint32_t> dynamicOffsets)
{
    if (!bindGroup)
    {
        TE_LOG_WARN("[RHIOpenGL] SetBindGroup called with null bindGroup");
        return;
    }

    (void)groupIndex;
    auto* glGroup = static_cast<OpenGLBindGroup*>(bindGroup);
    size_t dynamicOffsetIndex = 0;
    for (const auto& entry : glGroup->GetEntries())
    {
        switch (entry.type)
        {
        case RHIBindingType::UniformBuffer:
        {
            GLsizeiptr size = entry.bufferSize > 0
                ? static_cast<GLsizeiptr>(entry.bufferSize)
                : static_cast<GLsizeiptr>(0);

            if (size > 0)
            {
                glBindBufferRange(GL_UNIFORM_BUFFER, entry.binding,
                                  entry.glBuffer,
                                  static_cast<GLintptr>(entry.bufferOffset),
                                  size);
            }
            else
            {
                glBindBufferBase(GL_UNIFORM_BUFFER, entry.binding, entry.glBuffer);
            }
            break;
        }
        case RHIBindingType::DynamicUniformBuffer:
        {
            if (dynamicOffsetIndex >= dynamicOffsets.size() || entry.bufferSize == 0)
            {
                TE_LOG_ERROR("[RHIOpenGL] Dynamic uniform binding {} is missing a valid dynamic offset/range",
                             entry.binding);
                return;
            }

            const uint64_t finalOffset = entry.bufferOffset + dynamicOffsets[dynamicOffsetIndex++];
            glBindBufferRange(GL_UNIFORM_BUFFER,
                              entry.binding,
                              entry.glBuffer,
                              static_cast<GLintptr>(finalOffset),
                              static_cast<GLsizeiptr>(entry.bufferSize));
            break;
        }
        case RHIBindingType::Texture2D:
        case RHIBindingType::TextureCube:
        {
            glActiveTexture(GL_TEXTURE0 + entry.binding);
            glBindTexture(entry.glTextureTarget, entry.glTexture);
            if (entry.glSampler != 0)
            {
                glBindSampler(entry.binding, entry.glSampler);
            }
            else
            {
                glBindSampler(entry.binding, 0);
            }
            break;
        }
        case RHIBindingType::Sampler:
        {
            glBindSampler(entry.binding, entry.glSampler);
            break;
        }
        }
    }

    if (dynamicOffsetIndex != dynamicOffsets.size())
    {
        TE_LOG_WARN("[RHIOpenGL] SetBindGroup received {} unused dynamic offsets",
                    dynamicOffsets.size() - dynamicOffsetIndex);
    }
}

void OpenGLCommandBuffer::End()
{
    m_IsRecording = false;

    // 解绑状态（清理，防止状态泄漏）
    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLCommandBuffer::ApplyPipelineState()
{
    if (!m_BoundPipeline) return;

    const auto& raster = m_BoundPipeline->GetRasterizationDesc();
    const auto& depth = m_BoundPipeline->GetDepthStencilDesc();

    // 多边形模式
    switch (raster.polygonMode)
    {
        case RHIPolygonMode::Fill: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
        case RHIPolygonMode::Line: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
        case RHIPolygonMode::Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
    }

    // 面剔除
    switch (raster.cullMode)
    {
        case RHICullMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case RHICullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case RHICullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case RHICullMode::FrontAndBack:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
            break;
    }

    // 正面定义
    switch (raster.frontFace)
    {
        case RHIFrontFace::CounterClockwise: glFrontFace(GL_CCW); break;
        case RHIFrontFace::Clockwise:        glFrontFace(GL_CW); break;
    }

    // 深度测试
    if (depth.depthTestEnable)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(depth.depthWriteEnable ? GL_TRUE : GL_FALSE);

        switch (depth.depthCompareOp)
        {
            case RHICompareOp::Never:          glDepthFunc(GL_NEVER); break;
            case RHICompareOp::Less:           glDepthFunc(GL_LESS); break;
            case RHICompareOp::Equal:          glDepthFunc(GL_EQUAL); break;
            case RHICompareOp::LessOrEqual:    glDepthFunc(GL_LEQUAL); break;
            case RHICompareOp::Greater:        glDepthFunc(GL_GREATER); break;
            case RHICompareOp::NotEqual:       glDepthFunc(GL_NOTEQUAL); break;
            case RHICompareOp::GreaterOrEqual: glDepthFunc(GL_GEQUAL); break;
            case RHICompareOp::Always:         glDepthFunc(GL_ALWAYS); break;
        }
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    const auto& blendAttachments = m_BoundPipeline->GetRenderingDesc().colorBlendAttachments;
    if (blendAttachments.empty())
    {
        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        return;
    }

    for (uint32_t index = 0; index < blendAttachments.size(); ++index)
    {
        const auto& blend = blendAttachments[index];
        if (blend.blendEnable)
        {
            glEnablei(GL_BLEND, index);
            glBlendFuncSeparatei(index,
                                 BlendFactorToGL(blend.sourceColorFactor),
                                 BlendFactorToGL(blend.destinationColorFactor),
                                 BlendFactorToGL(blend.sourceAlphaFactor),
                                 BlendFactorToGL(blend.destinationAlphaFactor));
            glBlendEquationSeparatei(index,
                                     BlendOpToGL(blend.colorBlendOp),
                                     BlendOpToGL(blend.alphaBlendOp));
        }
        else
        {
            glDisablei(GL_BLEND, index);
        }

        glColorMaski(index,
                     HasAnyFlags(blend.writeMask, RHIColorWriteMask::Red),
                     HasAnyFlags(blend.writeMask, RHIColorWriteMask::Green),
                     HasAnyFlags(blend.writeMask, RHIColorWriteMask::Blue),
                     HasAnyFlags(blend.writeMask, RHIColorWriteMask::Alpha));
    }
}

} // namespace TE
