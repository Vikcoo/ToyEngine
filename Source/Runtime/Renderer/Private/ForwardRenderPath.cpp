// ToyEngine Renderer Module
// FForwardRenderPath 实现

#include "ForwardRenderPath.h"

#include "RendererLightUniforms.h"
#include "RendererScene.h"
#include "RenderStats.h"
#include "ViewInfo.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHIPipeline.h"
#include "RHITypes.h"

#include <algorithm>

namespace TE {

FForwardRenderPath::FForwardRenderPath()
    : m_BasePassProcessor(EMeshPassType::BasePass)
{
}

void FForwardRenderPath::Render(const FScene* scene,
                                RHIDevice* device,
                                RHICommandBuffer* cmdBuf,
                                FRenderStats& outStats)
{
    outStats = {};
    if (!scene || !device || !cmdBuf)
    {
        return;
    }

    std::vector<FMeshDrawCommand> drawCommands;
    m_BasePassProcessor.BuildDrawCommands(scene, drawCommands);
    if (drawCommands.empty())
    {
        return;
    }

    SortDrawCommands(drawCommands);

    const auto& viewInfo = scene->GetViewInfo();

    cmdBuf->Begin();

    RHIRenderPassBeginInfo passInfo;
    passInfo.clearColor[0] = 0.1f;
    passInfo.clearColor[1] = 0.1f;
    passInfo.clearColor[2] = 0.1f;
    passInfo.clearColor[3] = 1.0f;
    passInfo.clearDepth = 1.0f;
    passInfo.viewport.x = 0;
    passInfo.viewport.y = 0;
    passInfo.viewport.width = viewInfo.ViewportWidth;
    passInfo.viewport.height = viewInfo.ViewportHeight;

    cmdBuf->BeginRenderPass(passInfo);
    SubmitDrawCommands(drawCommands, scene, device, cmdBuf, outStats);
    cmdBuf->EndRenderPass();

    cmdBuf->End();
}

void FForwardRenderPath::SortDrawCommands(std::vector<FMeshDrawCommand>& commands) const
{
    std::sort(commands.begin(), commands.end(),
        [](const FMeshDrawCommand& a, const FMeshDrawCommand& b)
        {
            if (a.PipelineKey.Pass != b.PipelineKey.Pass)
                return static_cast<uint8_t>(a.PipelineKey.Pass) < static_cast<uint8_t>(b.PipelineKey.Pass);

            if (a.PipelineKey.MaterialDomain != b.PipelineKey.MaterialDomain)
                return static_cast<uint8_t>(a.PipelineKey.MaterialDomain) < static_cast<uint8_t>(b.PipelineKey.MaterialDomain);

            if (a.PipelineKey.VertexFactory != b.PipelineKey.VertexFactory)
                return static_cast<uint8_t>(a.PipelineKey.VertexFactory) < static_cast<uint8_t>(b.PipelineKey.VertexFactory);

            if (a.VertexBuffer != b.VertexBuffer)
                return a.VertexBuffer < b.VertexBuffer;

            return a.IndexBuffer < b.IndexBuffer;
        });
}

void FForwardRenderPath::SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                                            const FScene* scene,
                                            RHIDevice* device,
                                            RHICommandBuffer* cmdBuf,
                                            FRenderStats& outStats) const
{
    const auto& viewInfo = scene->GetViewInfo();

    const Matrix4 adjustedProjection = device->AdjustProjectionMatrix(viewInfo.ProjectionMatrix);
    const Matrix4 adjustedVP = adjustedProjection * viewInfo.ViewMatrix;

    RHIPipeline* lastPipeline = nullptr;
    RHIBuffer* lastVBO = nullptr;
    RHIBuffer* lastIBO = nullptr;

    for (const auto& cmd : commands)
    {
        auto* pipeline = scene->ResolvePreparedPipeline(cmd.PipelineKey);
        if (!pipeline || !pipeline->IsValid())
        {
            continue;
        }

        if (pipeline != lastPipeline)
        {
            cmdBuf->BindPipeline(pipeline);
            lastPipeline = pipeline;
            ++outStats.PipelineBindCount;

            lastVBO = nullptr;
            lastIBO = nullptr;
        }

        if (cmd.VertexBuffer != lastVBO)
        {
            cmdBuf->BindVertexBuffer(cmd.VertexBuffer);
            lastVBO = cmd.VertexBuffer;
            ++outStats.VBOBindCount;
        }

        if (cmd.IndexBuffer != lastIBO)
        {
            cmdBuf->BindIndexBuffer(cmd.IndexBuffer);
            lastIBO = cmd.IndexBuffer;
            ++outStats.IBOBindCount;
        }

        auto* baseColorTexture = scene->ResolvePreparedBaseColorTexture(cmd.StaticMeshAsset, cmd.MaterialIndex);
        auto* defaultSampler = scene->ResolveDefaultSampler();
        if (baseColorTexture)
        {
            cmdBuf->BindTexture2D(0, baseColorTexture, defaultSampler);
            cmdBuf->SetUniformInt("u_BaseColorTex", 0);
        }

        Matrix4 mvp = adjustedVP * cmd.WorldMatrix;

        cmdBuf->SetUniformMatrix4("u_MVP", mvp.Data());
        cmdBuf->SetUniformMatrix4("u_Model", cmd.WorldMatrix.Data());

        BindSceneLightUniforms(scene, cmdBuf);

        cmdBuf->DrawIndexed(cmd.IndexCount, cmd.FirstIndex);
        ++outStats.DrawCallCount;
    }
}

} // namespace TE
