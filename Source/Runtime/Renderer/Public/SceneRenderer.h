// ToyEngine Renderer Module
// SceneRenderer - 场景渲染调度器
// 对应 UE5 的 FSceneRenderer / FDeferredShadingSceneRenderer（简化版）
//
// 负责渲染的调度流程：
// 1. 从 FScene 获取所有 SceneProxy
// 2. 遍历每个 Proxy，调用 GetMeshDrawCommand() 收集绘制命令
// 3. 通过 RHI CommandBuffer 设置渲染状态并提交绘制
//
// 在 UE5 中，SceneRenderer 负责整个渲染管线（PrePass → BasePass → Lighting → PostProcess）
// 在 ToyEngine 单线程版本中，我们只实现最简单的 Forward 渲染（一个 Pass）

#pragma once

#include <vector>

namespace TE {

// 前向声明
class FScene;
class RHIDevice;
class RHICommandBuffer;
struct FMeshDrawCommand;
struct RHIRenderPassBeginInfo;

/// 场景渲染调度器
///
/// UE5 映射：
/// - FSceneRenderer::Render(): 核心渲染入口
/// - InitViews(): 可见性裁剪（本版本跳过）
/// - RenderBasePass(): 遍历 MeshDrawCommand 执行绘制
///
/// 单线程数据流：
/// Engine::Tick() → SceneRenderer::Render(FScene, Device, CmdBuf)
///   → 遍历 FScene::GetPrimitives()
///   → 每个 Proxy::GetMeshDrawCommand()
///   → CmdBuf::BindPipeline/BindVBO/BindIBO/SetUniform(MVP)/DrawIndexed
class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

    /// 渲染整个场景
    /// @param scene  渲染场景（包含 Proxy 列表和 ViewInfo）
    /// @param device RHI 设备（本版本未直接使用，留作扩展）
    /// @param cmdBuf RHI 命令缓冲区
    void Render(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf);

private:
    /// 收集所有 Proxy 的绘制命令
    void GatherMeshDrawCommands(const FScene* scene, std::vector<FMeshDrawCommand>& outCommands);

    /// 提交绘制命令到 RHI
    void SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                            const FScene* scene,
                            RHICommandBuffer* cmdBuf);
};

} // namespace TE
