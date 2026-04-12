// ToyEngine World Module
// 游戏侧到渲染侧的场景接口

#pragma once

#include "Math/MathTypes.h"
#include "PrimitiveComponentId.h"

#include <memory>

namespace TE {

class FPrimitiveSceneProxy;
class FStaticMeshRenderData;
class RHIPipeline;
class TStaticMesh;
class TPrimitiveComponent;

/// 游戏线程视角的渲染场景接口。
/// World 模块只依赖该接口，不直接依赖 Renderer 的具体实现类型。
class IRenderScene
{
public:
    virtual ~IRenderScene() = default;

    [[nodiscard]] virtual bool AddPrimitive(const TPrimitiveComponent* primitiveComponent,
                                            FPrimitiveComponentId primitiveComponentId,
                                            std::unique_ptr<FPrimitiveSceneProxy> proxy) = 0;
    virtual void UpdatePrimitiveTransform(FPrimitiveComponentId primitiveComponentId, const Matrix4& worldMatrix) = 0;
    virtual void RemovePrimitive(FPrimitiveComponentId primitiveComponentId) = 0;

    [[nodiscard]] virtual std::shared_ptr<const FStaticMeshRenderData> GetStaticMeshRenderData(
        const std::shared_ptr<TStaticMesh>& staticMesh) = 0;
    [[nodiscard]] virtual RHIPipeline* GetStaticMeshPipeline() = 0;
};

} // namespace TE
