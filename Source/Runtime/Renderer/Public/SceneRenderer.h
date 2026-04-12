// ToyEngine Renderer Module
// SceneRenderer - 场景渲染调度器
// 对应 UE5 的 FSceneRenderer / FDeferredShadingSceneRenderer（简化版）
//
// 负责渲染的调度流程：
// 1. 从 FScene 获取所有 SceneProxy
// 2. 遍历每个 Proxy，调用 GetMeshDrawCommand() 收集绘制命令
// 3. 按 Pipeline → VBO → IBO 排序（减少状态切换）
// 4. 通过 RHI CommandBuffer 设置渲染状态并提交绘制（跳过冗余绑定）
//
// 在 UE5 中，SceneRenderer 负责整个渲染管线（PrePass → BasePass → Lighting → PostProcess）
// 在 ToyEngine 单线程版本中，我们只实现最简单的 Forward 渲染（一个 Pass）
//
// Draw Call Batching 优化（UE5: FMeshDrawCommand Sort & Merge）：
// - 按 Pipeline 指针排序，使相同 Pipeline 的绘制命令相邻
// - 提交时跟踪上一次绑定的 Pipeline/VBO/IBO，跳过冗余绑定
// - 这减少了 glUseProgram / glBindVertexArray / 深度状态设置 等昂贵操作

#pragma once

#include <vector>
#include <cstdint>

namespace TE {

// 前向声明
class FScene;
class RHIDevice;
class RHICommandBuffer;
class RHIPipeline;
class RHIBuffer;
struct FMeshDrawCommand;
struct RHIRenderPassBeginInfo;

/// 场景渲染调度器
///
/// UE5 映射：
/// - FSceneRenderer::Render(): 核心渲染入口
/// - InitViews(): 可见性裁剪（本版本跳过）
/// - RenderBasePass(): 遍历 MeshDrawCommand 执行绘制
/// - SortMeshDrawCommands(): 按状态排序减少切换开销
///
/// 单线程数据流：
/// Engine::Tick() → SceneRenderer::Render(FScene, Device, CmdBuf)
///   → 遍历 FScene::GetPrimitives()
///   → 每个 Proxy::GetMeshDrawCommand()
///   → SortDrawCommands() ← 按 Pipeline/VBO/IBO 排序
///   → CmdBuf::BindPipeline/BindVBO/BindIBO/SetUniform(MVP)/DrawIndexed
///     （跳过与上一条相同的 Pipeline/VBO/IBO 绑定）
class FSceneRenderer
{
public:
    FSceneRenderer() = default;
    ~FSceneRenderer() = default;

    /// 渲染整个场景
    /// @param scene  渲染场景（包含 Proxy 列表和 ViewInfo）
    /// @param device RHI 设备（本版本未直接使用，留作扩展）
    /// @param cmdBuf RHI 命令缓冲区
    void Render(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf);

    /// 获取上一帧的 Draw Call 统计
    [[nodiscard]] uint32_t GetLastDrawCallCount() const { return m_LastDrawCallCount; }
    [[nodiscard]] uint32_t GetLastPipelineBindCount() const { return m_LastPipelineBindCount; }
    [[nodiscard]] uint32_t GetLastVBOBindCount() const { return m_LastVBOBindCount; }
    [[nodiscard]] uint32_t GetLastIBOBindCount() const { return m_LastIBOBindCount; }

private:
    /// 收集所有 Proxy 的绘制命令
    void GatherMeshDrawCommands(const FScene* scene, std::vector<FMeshDrawCommand>& outCommands);

    /// 按 Pipeline → VBO → IBO 排序绘制命令（减少状态切换）
    /// 对应 UE5 的 FMeshDrawCommand::SortByStateBuckets
    void SortDrawCommands(std::vector<FMeshDrawCommand>& commands);

    /// 提交绘制命令到 RHI（带冗余状态跳过优化）
    void SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                            const FScene* scene,
                            RHICommandBuffer* cmdBuf);

    // ==================== 帧统计 ====================
    uint32_t m_LastDrawCallCount = 0;       // 上一帧的 Draw Call 数量
    uint32_t m_LastPipelineBindCount = 0;   // 上一帧的 Pipeline 绑定次数
    uint32_t m_LastVBOBindCount = 0;        // 上一帧的 VBO 绑定次数
    uint32_t m_LastIBOBindCount = 0;        // 上一帧的 IBO 绑定次数
};

} // namespace TE
