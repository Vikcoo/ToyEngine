// ToyEngine RenderCore Module
// FStaticMeshSceneProxy - 静态网格的渲染侧镜像
// 对应 UE5 的 FStaticMeshSceneProxy

#pragma once

#include "PrimitiveSceneProxy.h"
#include "StaticMeshRenderData.h"

#include <memory>

namespace TE {

class StaticMesh;

class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
    explicit FStaticMeshSceneProxy(std::shared_ptr<StaticMesh> staticMesh);
    ~FStaticMeshSceneProxy() override;

    [[nodiscard]] const std::shared_ptr<StaticMesh>& GetStaticMeshAsset() const { return m_StaticMesh; }
    [[nodiscard]] bool HasStaticMeshAsset() const { return m_StaticMesh != nullptr; }
    [[nodiscard]] bool SetRenderResources(std::shared_ptr<const FStaticMeshRenderData> renderData);

    void GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const override;

    [[nodiscard]] bool IsValid() const;

private:
    std::shared_ptr<StaticMesh> m_StaticMesh;
    std::shared_ptr<const FStaticMeshRenderData> m_RenderData;
};

} // namespace TE
