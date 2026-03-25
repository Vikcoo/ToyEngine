// ToyEngine Asset Module
// TStaticMesh 实现

#include "TStaticMesh.h"

namespace TE {

bool TStaticMesh::IsValid() const
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

uint32_t TStaticMesh::GetTotalVertexCount() const
{
    uint32_t total = 0;
    for (const auto& section : m_Sections)
    {
        total += static_cast<uint32_t>(section.Vertices.size());
    }
    return total;
}

uint32_t TStaticMesh::GetTotalIndexCount() const
{
    uint32_t total = 0;
    for (const auto& section : m_Sections)
    {
        total += static_cast<uint32_t>(section.Indices.size());
    }
    return total;
}

void TStaticMesh::AddSection(FMeshSection section)
{
    m_Sections.push_back(std::move(section));
}

} // namespace TE
