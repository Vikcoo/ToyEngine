// ToyEngine RenderCore Module
// FStaticMeshSceneProxy 实现

#include "StaticMeshSceneProxy.h"

#include "TStaticMesh.h"

namespace TE {

FStaticMeshSceneProxy::FStaticMeshSceneProxy(std::shared_ptr<StaticMesh> staticMesh)
    : m_StaticMesh(std::move(staticMesh))
{
}

FStaticMeshSceneProxy::~FStaticMeshSceneProxy() = default;

bool FStaticMeshSceneProxy::SetRenderResources(std::shared_ptr<const FStaticMeshRenderData> renderData)
{
    if (!renderData || !renderData->IsValid())
    {
        return false;
    }

    m_RenderData = std::move(renderData);
    return true;
}

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
        cmd.PipelineKey = EMeshPipelineKey::StaticMeshLit;
        cmd.VertexBuffer = vertexBuffer;
        cmd.IndexBuffer = indexBuffer;
        cmd.FirstIndex = section.FirstIndex;
        cmd.IndexCount = section.IndexCount;
        cmd.MaterialIndex = section.MaterialIndex;
        cmd.WorldMatrix = m_WorldMatrix;
        outCommands.push_back(cmd);
    }
}

bool FStaticMeshSceneProxy::IsValid() const
{
    return m_StaticMesh && m_StaticMesh->IsValid() &&
           m_RenderData && m_RenderData->IsValid();
}

} // namespace TE
