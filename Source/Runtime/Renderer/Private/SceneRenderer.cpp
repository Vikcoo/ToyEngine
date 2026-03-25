// ToyEngine Renderer Module
// SceneRenderer 实现 - 核心渲染循环（带 Draw Call Batching 优化）
//
// UE5 渲染流程（简化版）：
// 1. InitViews:            可见性裁剪（本版本跳过）
// 2. GatherMeshDrawCommands: 遍历 Proxy 收集 DrawCommand
// 3. SortDrawCommands:      按 Pipeline/VBO/IBO 排序（减少状态切换）
// 4. SubmitDrawCommands:    逐命令设置 RHI 状态并绘制（跳过冗余绑定）
//
// Draw Call Batching 优化原理（对应 UE5 Mesh Draw Pipeline 的 Sort & State Filtering）：
//
// 优化前（Naive）：每条 DrawCommand 都执行完整的：
//   BindPipeline → BindVBO → BindIBO → SetUniform → DrawIndexed
//
// 优化后（Batching）：
//   1. 先按 Pipeline 指针排序，使相同 Pipeline 的命令连续
//   2. 提交时跟踪 lastBoundPipeline / lastBoundVBO / lastBoundIBO
//   3. 仅在指针变化时才执行绑定操作
//
// 对于一个 10 Section 的模型（都用同一 Pipeline）：
//   优化前：10 次 BindPipeline + 10 次 BindVBO + 10 次 BindIBO = 30 次绑定
//   优化后：1 次 BindPipeline + 10 次 BindVBO + 10 次 BindIBO = 21 次绑定
//   （Pipeline 绑定包含 glUseProgram + glBindVAO + 深度/光栅状态设置，是最昂贵的操作）
//
// 对于多个对象共享相同 Pipeline（场景中有多个 StaticMesh）：
//   排序后它们会被分组到一起，Pipeline 切换次数降到最少

#include "SceneRenderer.h"
#include "FScene.h"
#include "FPrimitiveSceneProxy.h"
#include "FMeshDrawCommand.h"
#include "FViewInfo.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHITypes.h"
#include "Log/Log.h"

#include <algorithm>

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
    {
        m_LastDrawCallCount = 0;
        m_LastPipelineBindCount = 0;
        m_LastVBOBindCount = 0;
        m_LastIBOBindCount = 0;
        return;
    }

    // === Step 2: 按 Pipeline 排序（UE5: SortMeshDrawCommands） ===
    SortDrawCommands(drawCommands);

    // === Step 3: 开始录制 RHI 命令 ===
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

    // === Step 4: 提交绘制命令（UE5: RenderBasePass，带冗余状态跳过） ===
    SubmitDrawCommands(drawCommands, scene, cmdBuf);

    cmdBuf->EndRenderPass();
    cmdBuf->End();
}

void SceneRenderer::GatherMeshDrawCommands(const FScene* scene,
                                            std::vector<FMeshDrawCommand>& outCommands)
{
    const auto& primitives = scene->GetPrimitives();
    outCommands.reserve(primitives.size() * 2); // 预估每个 Proxy 平均 2 个 Section

    for (const auto* proxy : primitives)
    {
        proxy->GetMeshDrawCommands(outCommands);
    }
}

void SceneRenderer::SortDrawCommands(std::vector<FMeshDrawCommand>& commands)
{
    // 按 Pipeline → VBO → IBO 指针值排序
    // 这保证了相同 Pipeline 的命令连续，相同 VBO/IBO 的命令也尽量连续
    //
    // UE5 映射：FMeshDrawCommand::SortByStateBuckets()
    // UE5 中排序键包括：PipelineId / StencilRef / MeshId / ShaderBinding 等
    // ToyEngine 简化版只按 Pipeline / VBO / IBO 地址排序
    std::sort(commands.begin(), commands.end(),
        [](const FMeshDrawCommand& a, const FMeshDrawCommand& b)
        {
            // 首先按 Pipeline 分组（最昂贵的状态切换）
            if (a.Pipeline != b.Pipeline)
                return a.Pipeline < b.Pipeline;

            // 同 Pipeline 内按 VBO 分组
            if (a.VertexBuffer != b.VertexBuffer)
                return a.VertexBuffer < b.VertexBuffer;

            // 同 VBO 内按 IBO 分组
            return a.IndexBuffer < b.IndexBuffer;
        });
}

void SceneRenderer::SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                                        const FScene* scene,
                                        RHICommandBuffer* cmdBuf)
{
    const auto& viewInfo = scene->GetViewInfo();

    // ==================== Draw Call Batching 状态跟踪 ====================
    // 跟踪上一次绑定的资源，跳过冗余绑定
    // 对应 UE5 中 FMeshDrawCommand::SubmitDraw() 的状态过滤逻辑
    RHIPipeline* lastPipeline = nullptr;
    RHIBuffer*   lastVBO      = nullptr;
    RHIBuffer*   lastIBO      = nullptr;

    // 帧统计计数器
    uint32_t drawCallCount    = 0;
    uint32_t pipelineBinds    = 0;
    uint32_t vboBinds         = 0;
    uint32_t iboBinds         = 0;

    for (const auto& cmd : commands)
    {
        // --- Pipeline 绑定（仅在 Pipeline 变化时执行） ---
        // BindPipeline 包含 glUseProgram + glBindVAO + 光栅化/深度状态设置
        // 这是最昂贵的操作，排序后同 Pipeline 的命令连续，只需绑定一次
        if (cmd.Pipeline != lastPipeline)
        {
            cmdBuf->BindPipeline(cmd.Pipeline);
            lastPipeline = cmd.Pipeline;
            ++pipelineBinds;

            // Pipeline 变更后，VBO/IBO 绑定需要重新执行
            // 因为 VAO 切换后，之前的 VBO/IBO 关联失效
            lastVBO = nullptr;
            lastIBO = nullptr;
        }

        // --- VBO 绑定（仅在 VBO 变化时执行） ---
        if (cmd.VertexBuffer != lastVBO)
        {
            cmdBuf->BindVertexBuffer(cmd.VertexBuffer);
            lastVBO = cmd.VertexBuffer;
            ++vboBinds;
        }

        // --- IBO 绑定（仅在 IBO 变化时执行） ---
        if (cmd.IndexBuffer != lastIBO)
        {
            cmdBuf->BindIndexBuffer(cmd.IndexBuffer);
            lastIBO = cmd.IndexBuffer;
            ++iboBinds;
        }

        // --- Uniform 设置（每个物体不同，不能跳过） ---
        // 计算 MVP = Projection * View * Model
        Matrix4 mvp = viewInfo.ViewProjectionMatrix * cmd.WorldMatrix;

        // 设置变换矩阵 Uniform
        cmdBuf->SetUniformMatrix4("u_MVP", mvp.Data());
        cmdBuf->SetUniformMatrix4("u_Model", cmd.WorldMatrix.Data());

        // 设置光照 Uniform（方向光）
        // 光照方向：从右上方照射（归一化）
        Vector3 lightDir = Vector3(0.5f, 1.0f, 0.8f).Normalize();
        cmdBuf->SetUniformVec3("u_LightDir", &lightDir.X);

        // 光源颜色：温暖的白色
        Vector3 lightColor(1.0f, 0.95f, 0.9f);
        cmdBuf->SetUniformVec3("u_LightColor", &lightColor.X);

        // --- 提交绘制 ---
        cmdBuf->DrawIndexed(cmd.IndexCount);
        ++drawCallCount;
    }

    // 更新帧统计
    m_LastDrawCallCount = drawCallCount;
    m_LastPipelineBindCount = pipelineBinds;
    m_LastVBOBindCount = vboBinds;
    m_LastIBOBindCount = iboBinds;
}

} // namespace TE
