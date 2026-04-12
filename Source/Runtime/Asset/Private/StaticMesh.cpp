// ToyEngine Asset Module
// TStaticMesh 实现

#include "StaticMesh.h"

namespace TE {

bool StaticMesh::IsValid() const
{
    if (m_Sections.empty())
        return false;

    for (const auto& section : m_Sections)
    {
        if (!section.Vertices.empty() && !section.Indices.empty())
            return true;
    }
    return false;
}

uint32_t StaticMesh::GetTotalVertexCount() const
{
    uint32_t total = 0;
    for (const auto& section : m_Sections)
    {
        total += static_cast<uint32_t>(section.Vertices.size());
    }
    return total;
}

uint32_t StaticMesh::GetTotalIndexCount() const
{
    uint32_t total = 0;
    for (const auto& section : m_Sections)
    {
        total += static_cast<uint32_t>(section.Indices.size());
    }
    return total;
}

void StaticMesh::AddSection(FMeshSection section)
{
    m_Sections.push_back(std::move(section));
}

const FStaticMeshMaterial* StaticMesh::GetMaterial(uint32_t materialIndex) const
{
    if (materialIndex >= m_Materials.size())
    {
        return nullptr;
    }
    return &m_Materials[materialIndex];
}

} // namespace TE
