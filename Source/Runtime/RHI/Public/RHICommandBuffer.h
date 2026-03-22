// ToyEngine RHI Module
// RHI CommandBuffer 抽象接口 - 封装 GPU 命令录制
// 设计参考 Vulkan VkCommandBuffer / D3D12 ID3D12GraphicsCommandList

#pragma once

#include "RHITypes.h"

namespace TE {

/// 命令缓冲区抽象接口
///
/// 使用模式（参考 Vulkan 命令缓冲区生命周期）:
/// 1. Begin()                      — 开始录制命令
/// 2. BeginRenderPass(info)        — 开始渲染通道（设置清屏、视口）
/// 3. BindPipeline(pipeline)       — 绑定图形管线
/// 4. BindVertexBuffer(buffer)     — 绑定顶点缓冲区
/// 5. SetViewport(viewport)        — 动态设置视口（可选）
/// 6. SetScissor(rect)             — 动态设置裁剪矩形（可选）
/// 7. Draw(vertexCount) 或 DrawIndexed(indexCount)  — 提交绘制命令
/// 8. EndRenderPass()              — 结束渲染通道
/// 9. End()                        — 结束录制
///
/// OpenGL 后端：命令在录制时立即执行（immediate mode）
/// Vulkan 后端：命令真正录入 VkCommandBuffer，在 Submit 时提交给 GPU
/// D3D12 后端：命令录入 CommandList，在 Close + ExecuteCommandLists 时提交
class RHICommandBuffer
{
public:
    virtual ~RHICommandBuffer() = default;

    /// 开始录制命令
    virtual void Begin() = 0;

    /// 开始渲染通道
    virtual void BeginRenderPass(const RHIRenderPassBeginInfo& info) = 0;

    /// 结束渲染通道
    virtual void EndRenderPass() = 0;

    /// 绑定图形管线
    virtual void BindPipeline(RHIPipeline* pipeline) = 0;

    /// 绑定顶点缓冲区
    /// @param buffer  顶点缓冲区
    /// @param binding 绑定点索引（对应顶点输入布局中的 binding）
    /// @param offset  缓冲区内偏移（字节）
    virtual void BindVertexBuffer(RHIBuffer* buffer, uint32_t binding = 0, uint64_t offset = 0) = 0;

    /// 绑定索引缓冲区
    /// @param buffer    索引缓冲区
    /// @param indexType 索引类型（UInt16 / UInt32）
    /// @param offset    缓冲区内偏移（字节）
    virtual void BindIndexBuffer(RHIBuffer* buffer, RHIIndexType indexType = RHIIndexType::UInt32, uint64_t offset = 0) = 0;

    /// 动态设置视口
    virtual void SetViewport(const RHIViewport& viewport) = 0;

    /// 动态设置裁剪矩形
    virtual void SetScissor(const RHIScissorRect& scissor) = 0;

    /// 非索引绘制
    /// @param vertexCount 顶点数量
    /// @param firstVertex 起始顶点索引
    /// @param instanceCount 实例数量
    /// @param firstInstance 起始实例索引
    virtual void Draw(uint32_t vertexCount, uint32_t firstVertex = 0,
                      uint32_t instanceCount = 1, uint32_t firstInstance = 0) = 0;

    /// 索引绘制
    /// @param indexCount    索引数量
    /// @param firstIndex   起始索引偏移
    /// @param vertexOffset 顶点偏移
    /// @param instanceCount 实例数量
    /// @param firstInstance 起始实例索引
    virtual void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0,
                             int32_t vertexOffset = 0,
                             uint32_t instanceCount = 1, uint32_t firstInstance = 0) = 0;

    // ==================== Uniform 设置 ====================
    // 设计说明：Uniform 接口目前暴露 name-based 设置，匹配 OpenGL uniform 模型。
    // Vulkan/D3D12 后端将来改为 descriptor set / root signature 方式，
    // 届时这些接口的实现会变为查表写入 UBO。

    /// 设置 4x4 矩阵 Uniform
    /// @param name Uniform 变量名（OpenGL 用 glGetUniformLocation 查找）
    /// @param data 4x4 矩阵数据指针（列主序，16 个 float）
    virtual void SetUniformMatrix4(const char* name, const float* data) = 0;

    /// 设置 float Uniform
    /// @param name Uniform 变量名
    /// @param value 浮点值
    virtual void SetUniformFloat(const char* name, float value) = 0;

    /// 结束录制命令
    virtual void End() = 0;

protected:
    RHICommandBuffer() = default;
};

} // namespace TE
