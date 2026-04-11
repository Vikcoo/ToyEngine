// ToyEngine RenderCore Module
// FStaticMeshRenderData - 资产级静态网格渲染数据

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace TE {

class RHIBuffer;
class RHIDevice;
class TStaticMesh;

struct FStaticMeshSectionRange
{
    uint32_t FirstIndex = 0;
    uint32_t IndexCount = 0;
    uint32_t MaterialIndex = 0;
};

class FStaticMeshRenderData
{
public:
    FStaticMeshRenderData() = default;
    ~FStaticMeshRenderData() = default;

    FStaticMeshRenderData(const FStaticMeshRenderData&) = delete;
    FStaticMeshRenderData& operator=(const FStaticMeshRenderData&) = delete;

    [[nodiscard]] static std::shared_ptr<FStaticMeshRenderData> Create(const TStaticMesh& staticMesh, RHIDevice& device);

    [[nodiscard]] bool IsValid() const;

    [[nodiscard]] RHIBuffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
    [[nodiscard]] RHIBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }
    [[nodiscard]] const std::vector<FStaticMeshSectionRange>& GetSections() const { return m_Sections; }

private:
    std::unique_ptr<RHIBuffer> m_VertexBuffer;
    std::unique_ptr<RHIBuffer> m_IndexBuffer;
    std::vector<FStaticMeshSectionRange> m_Sections;
};

} // namespace TE
