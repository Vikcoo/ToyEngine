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

    // ==================== 资源绑定 ====================

    /// 绑定资源组到指定槽位
    /// @param groupIndex 绑定组索引（对应 Vulkan descriptor set index / D3D12 root parameter index）
    /// @param bindGroup  资源绑定组
    virtual void SetBindGroup(uint32_t groupIndex, RHIBindGroup* bindGroup) = 0;

    // ==================== 旧版 Uniform 接口（兼容过渡期） ====================
    // 这些 name-based 接口仅供 OpenGL 后端在过渡期使用。
    // Vulkan/D3D12 后端应通过 BindGroup 内的 UBO 设置 Uniform 数据。
    // 后续将逐步移除这些接口，全部迁移到 BindGroup 模型。

    virtual void SetUniformMatrix4(const char* name, const float* data) = 0;
    virtual void SetUniformMatrix3(const char* name, const float* data) = 0;
    virtual void SetUniformFloat(const char* name, float value) = 0;
    virtual void SetUniformVec3(const char* name, const float* data) = 0;
    virtual void SetUniformInt(const char* name, int32_t value) = 0;

    /// 绑定 2D 纹理与采样器到指定槽位（旧版接口，建议使用 BindGroup）
    virtual void BindTexture2D(uint32_t slot, RHITexture* texture, RHISampler* sampler = nullptr) = 0;

    /// 结束录制命令
    virtual void End() = 0;

protected:
    RHICommandBuffer() = default;
};

} // namespace TE
