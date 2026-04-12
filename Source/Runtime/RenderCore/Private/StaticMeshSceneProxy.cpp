// ToyEngine RenderCore Module
// FStaticMeshSceneProxy 实现

#include "StaticMeshSceneProxy.h"

#include "RHIPipeline.h"

namespace TE {

FStaticMeshSceneProxy::FStaticMeshSceneProxy(std::shared_ptr<const FStaticMeshRenderData> renderData, RHIPipeline* pipeline)
    : m_RenderData(std::move(renderData))
    , m_Pipeline(pipeline)
{
}

FStaticMeshSceneProxy::~FStaticMeshSceneProxy() = default;

void FStaticMeshSceneProxy::GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const
{
    if (!IsValid())
    {
        return;
    }

    auto* vertexBuffer = m_RenderData->GetVertexBuffer();
    auto* indexBuffer = m_RenderData->GetIndexBuffer();
    if (!vertexBuffer || !indexBuffer)
    {
        return;
    }

    for (const auto& section : m_RenderData->GetSections())
    {
        FMeshDrawCommand cmd;
        cmd.Pipeline = m_Pipeline;
        cmd.VertexBuffer = vertexBuffer;
        cmd.IndexBuffer = indexBuffer;
        cmd.FirstIndex = section.FirstIndex;
        cmd.IndexCount = section.IndexCount;
        cmd.WorldMatrix = m_WorldMatrix;
        outCommands.push_back(cmd);
    }
}

bool FStaticMeshSceneProxy::IsValid() const
{
    return m_RenderData && m_RenderData->IsValid() && m_Pipeline && m_Pipeline->IsValid();
}

} // namespace TE
