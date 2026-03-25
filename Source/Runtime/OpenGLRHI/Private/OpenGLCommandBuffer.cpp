// ToyEngine OpenGLRHI Module
// OpenGL CommandBuffer 实现 - Immediate mode 执行

#include "OpenGLCommandBuffer.h"
#include "OpenGLPipeline.h"
#include "OpenGLBuffer.h"
#include "Log/Log.h"

namespace TE {

void OpenGLCommandBuffer::Begin()
{
    m_IsRecording = true;
    m_BoundPipeline = nullptr;
}

void OpenGLCommandBuffer::BeginRenderPass(const RHIRenderPassBeginInfo& info)
{
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

    // 清屏
    glClearColor(info.clearColor[0], info.clearColor[1], info.clearColor[2], info.clearColor[3]);
    glClearDepth(info.clearDepth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLCommandBuffer::EndRenderPass()
{
    // OpenGL 中 RenderPass 不需要显式结束操作
    // Vulkan/D3D12 后端在此处执行 vkCmdEndRenderPass / OMSetRenderTargets(null)
}

void OpenGLCommandBuffer::BindPipeline(RHIPipeline* pipeline)
{
    if (!pipeline)
    {
        TE_LOG_WARN("[OpenGLRHI] BindPipeline called with null pipeline");
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
        TE_LOG_WARN("[OpenGLRHI] BindVertexBuffer called with null buffer");
        return;
    }

    if (!m_BoundPipeline)
    {
        TE_LOG_WARN("[OpenGLRHI] BindVertexBuffer called without bound pipeline");
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
        TE_LOG_WARN("[OpenGLRHI] BindIndexBuffer called with null buffer");
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

void OpenGLCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex,
                                uint32_t instanceCount, uint32_t firstInstance)
{
    if (!m_BoundPipeline)
    {
        TE_LOG_ERROR("[OpenGLRHI] Draw called without bound pipeline");
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
        TE_LOG_ERROR("[OpenGLRHI] DrawIndexed called without bound pipeline");
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

void OpenGLCommandBuffer::SetUniformMatrix4(const char* name, const float* data)
{
    if (!m_BoundPipeline)
    {
        TE_LOG_WARN("[OpenGLRHI] SetUniformMatrix4 called without bound pipeline");
        return;
    }

    GLint location = glGetUniformLocation(m_BoundPipeline->GetGLProgram(), name);
    if (location == -1)
    {
        TE_LOG_WARN("[OpenGLRHI] Uniform '{}' not found in shader program", name);
        return;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, data);
}

void OpenGLCommandBuffer::SetUniformFloat(const char* name, float value)
{
    if (!m_BoundPipeline)
    {
        TE_LOG_WARN("[OpenGLRHI] SetUniformFloat called without bound pipeline");
        return;
    }

    GLint location = glGetUniformLocation(m_BoundPipeline->GetGLProgram(), name);
    if (location == -1)
    {
        TE_LOG_WARN("[OpenGLRHI] Uniform '{}' not found in shader program", name);
        return;
    }
    glUniform1f(location, value);
}

void OpenGLCommandBuffer::SetUniformVec3(const char* name, const float* data)
{
    if (!m_BoundPipeline)
    {
        TE_LOG_WARN("[OpenGLRHI] SetUniformVec3 called without bound pipeline");
        return;
    }

    GLint location = glGetUniformLocation(m_BoundPipeline->GetGLProgram(), name);
    if (location == -1)
    {
        TE_LOG_WARN("[OpenGLRHI] Uniform '{}' not found in shader program", name);
        return;
    }
    glUniform3fv(location, 1, data);
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
}

} // namespace TE
