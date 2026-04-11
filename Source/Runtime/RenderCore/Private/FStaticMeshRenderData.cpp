// ToyEngine RenderCore Module
// FStaticMeshRenderData 实现

#include "FStaticMeshRenderData.h"

#include "RHIBuffer.h"
#include "RHIDevice.h"
#include "RHITypes.h"
#include "TStaticMesh.h"

namespace TE {

std::shared_ptr<FStaticMeshRenderData> FStaticMeshRenderData::Create(const TStaticMesh& staticMesh, RHIDevice& device)
{
    if (!staticMesh.IsValid())
    {
        return nullptr;
    }

    auto renderData = std::make_shared<FStaticMeshRenderData>();

    std::vector<FStaticMeshVertex> packedVertices;
    std::vector<uint32_t> packedIndices;
    std::vector<FStaticMeshSectionRange> sections;

    packedVertices.reserve(staticMesh.GetTotalVertexCount());
    packedIndices.reserve(staticMesh.GetTotalIndexCount());
    sections.reserve(staticMesh.GetSectionCount());

    for (const auto& section : staticMesh.GetSections())
    {
        if (section.Vertices.empty() || section.Indices.empty())
        {
            continue;
        }

        const uint32_t vertexBase = static_cast<uint32_t>(packedVertices.size());
        const uint32_t firstIndex = static_cast<uint32_t>(packedIndices.size());

        packedVertices.insert(packedVertices.end(), section.Vertices.begin(), section.Vertices.end());
        packedIndices.reserve(packedIndices.size() + section.Indices.size());
        for (uint32_t index : section.Indices)
        {
            packedIndices.push_back(index + vertexBase);
        }

        FStaticMeshSectionRange range;
        range.FirstIndex = firstIndex;
        range.IndexCount = static_cast<uint32_t>(section.Indices.size());
        range.MaterialIndex = section.MaterialIndex;
        sections.push_back(range);
    }

    if (packedVertices.empty() || packedIndices.empty() || sections.empty())
    {
        return nullptr;
    }

    RHIBufferDesc vbDesc;
    vbDesc.usage = RHIBufferUsage::Vertex;
    vbDesc.size = packedVertices.size() * sizeof(FStaticMeshVertex);
    vbDesc.initialData = packedVertices.data();
    vbDesc.debugName = "StaticMesh_Asset_VBO";
    renderData->m_VertexBuffer = device.CreateBuffer(vbDesc);
    if (!renderData->m_VertexBuffer)
    {
        return nullptr;
    }

    RHIBufferDesc ibDesc;
    ibDesc.usage = RHIBufferUsage::Index;
    ibDesc.size = packedIndices.size() * sizeof(uint32_t);
    ibDesc.initialData = packedIndices.data();
    ibDesc.debugName = "StaticMesh_Asset_IBO";
    renderData->m_IndexBuffer = device.CreateBuffer(ibDesc);
    if (!renderData->m_IndexBuffer)
    {
        return nullptr;
    }

    renderData->m_Sections = std::move(sections);
    return renderData;
}

bool FStaticMeshRenderData::IsValid() const
{
    return m_VertexBuffer != nullptr && m_IndexBuffer != nullptr && !m_Sections.empty();
}

} // namespace TE
