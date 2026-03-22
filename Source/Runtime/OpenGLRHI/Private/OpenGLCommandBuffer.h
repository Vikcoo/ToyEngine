// ToyEngine OpenGLRHI Module
// OpenGL CommandBuffer 实现
// OpenGL 没有真正的命令缓冲区，采用 immediate mode：
// 命令在录制时直接执行对应的 GL 调用

#pragma once

#include "RHICommandBuffer.h"
#include <glad/glad.h>

namespace TE {

class OpenGLPipeline;

class OpenGLCommandBuffer final : public RHICommandBuffer
{
public:
    OpenGLCommandBuffer() = default;
    ~OpenGLCommandBuffer() override = default;

    // 禁止拷贝
    OpenGLCommandBuffer(const OpenGLCommandBuffer&) = delete;
    OpenGLCommandBuffer& operator=(const OpenGLCommandBuffer&) = delete;

    void Begin() override;
    void BeginRenderPass(const RHIRenderPassBeginInfo& info) override;
    void EndRenderPass() override;
    void BindPipeline(RHIPipeline* pipeline) override;
    void BindVertexBuffer(RHIBuffer* buffer, uint32_t binding = 0, uint64_t offset = 0) override;
    void BindIndexBuffer(RHIBuffer* buffer, RHIIndexType indexType = RHIIndexType::UInt32, uint64_t offset = 0) override;
    void SetViewport(const RHIViewport& viewport) override;
    void SetScissor(const RHIScissorRect& scissor) override;
    void Draw(uint32_t vertexCount, uint32_t firstVertex = 0,
              uint32_t instanceCount = 1, uint32_t firstInstance = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0,
                     int32_t vertexOffset = 0,
                     uint32_t instanceCount = 1, uint32_t firstInstance = 0) override;
    void SetUniformMatrix4(const char* name, const float* data) override;
    void SetUniformFloat(const char* name, float value) override;
    void End() override;

private:
    /// 应用管线的光栅化和深度模板状态到 OpenGL
    void ApplyPipelineState();

    OpenGLPipeline* m_BoundPipeline = nullptr;
    bool            m_IsRecording = false;
};

} // namespace TE
