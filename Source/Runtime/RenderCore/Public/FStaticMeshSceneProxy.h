// ToyEngine RenderCore Module
// FStaticMeshSceneProxy - 静态网格的渲染侧镜像
// 对应 UE5 的 FStaticMeshSceneProxy

#pragma once

#include "FPrimitiveSceneProxy.h"
#include "FStaticMeshRenderData.h"

#include <memory>

namespace TE {

class RHIPipeline;

class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
    FStaticMeshSceneProxy(std::shared_ptr<const FStaticMeshRenderData> renderData, RHIPipeline* pipeline);
    ~FStaticMeshSceneProxy() override;

    void GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const override;

    [[nodiscard]] bool IsValid() const;

private:
    std::shared_ptr<const FStaticMeshRenderData> m_RenderData;
    RHIPipeline* m_Pipeline = nullptr;
};

} // namespace TE
