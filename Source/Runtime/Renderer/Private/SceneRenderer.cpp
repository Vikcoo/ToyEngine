// ToyEngine Renderer Module
// SceneRenderer 实现 - 核心渲染循环
//
// UE5 渲染流程（简化版）：
// 1. InitViews:      可见性裁剪（本版本跳过）
// 2. GatherMeshDrawCommands: 遍历 Proxy 收集 DrawCommand
// 3. SubmitDrawCommands:     逐命令设置 RHI 状态并绘制
//
// 这就是 UE5 Mesh Draw Pipeline 的最简化版本

#include "SceneRenderer.h"
#include "FScene.h"
#include "FPrimitiveSceneProxy.h"
#include "FMeshDrawCommand.h"
#include "FViewInfo.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHITypes.h"
#include "Log/Log.h"

namespace TE {

void SceneRenderer::Render(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf)
{
    if (!scene || !cmdBuf)
    {
        TE_LOG_ERROR("[Renderer] SceneRenderer::Render called with null scene or cmdBuf");
        return;
    }

    // === Step 1: 收集绘制命令（UE5: GatherMeshDrawCommands） ===
    std::vector<FMeshDrawCommand> drawCommands;
    GatherMeshDrawCommands(scene, drawCommands);

    if (drawCommands.empty())
        return;

    // === Step 2: 开始录制 RHI 命令 ===
    const auto& viewInfo = scene->GetViewInfo();

    cmdBuf->Begin();

    // 设置渲染通道（清屏 + 视口）
    RHIRenderPassBeginInfo passInfo;
    passInfo.clearColor[0] = 0.1f;  // 深灰色背景
    passInfo.clearColor[1] = 0.1f;
    passInfo.clearColor[2] = 0.1f;
    passInfo.clearColor[3] = 1.0f;
    passInfo.clearDepth = 1.0f;
    passInfo.viewport.x = 0;
    passInfo.viewport.y = 0;
    passInfo.viewport.width = viewInfo.ViewportWidth;
    passInfo.viewport.height = viewInfo.ViewportHeight;

    cmdBuf->BeginRenderPass(passInfo);

    // === Step 3: 提交绘制命令（UE5: RenderBasePass） ===
    SubmitDrawCommands(drawCommands, scene, cmdBuf);

    cmdBuf->EndRenderPass();
    cmdBuf->End();
}

void SceneRenderer::GatherMeshDrawCommands(const FScene* scene,
                                            std::vector<FMeshDrawCommand>& outCommands)
{
    const auto& primitives = scene->GetPrimitives();
    outCommands.reserve(primitives.size());

    for (const auto* proxy : primitives)
    {
        FMeshDrawCommand cmd;
        if (proxy->GetMeshDrawCommand(cmd))
        {
            outCommands.push_back(cmd);
        }
    }
}

void SceneRenderer::SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                                        const FScene* scene,
                                        RHICommandBuffer* cmdBuf)
{
    const auto& viewInfo = scene->GetViewInfo();

    for (const auto& cmd : commands)
    {
        // 绑定管线
        cmdBuf->BindPipeline(cmd.Pipeline);

        // 绑定顶点缓冲区
        cmdBuf->BindVertexBuffer(cmd.VertexBuffer);

        // 绑定索引缓冲区
        cmdBuf->BindIndexBuffer(cmd.IndexBuffer);

        // 计算 MVP = Projection * View * Model
        Matrix4 mvp = viewInfo.ViewProjectionMatrix * cmd.WorldMatrix;

        // 设置 Uniform
        cmdBuf->SetUniformMatrix4("u_MVP", mvp.Data());

        // 提交绘制
        cmdBuf->DrawIndexed(cmd.IndexCount);
    }
}

} // namespace TE
