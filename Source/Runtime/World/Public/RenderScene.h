// ToyEngine World Module
// 游戏侧到渲染侧的场景接口

#pragma once

#include "Math/MathTypes.h"
#include "LightComponentId.h"
#include "PrimitiveComponentId.h"

#include <memory>

namespace TE {

class FPrimitiveSceneProxy;
struct FLightSceneProxy;
class LightComponent;
class PrimitiveComponent;

/// 游戏线程视角的渲染场景接口。
/// World 模块只依赖该接口，不直接依赖 Renderer 的具体实现类型。
class IRenderScene
{
public:
    virtual ~IRenderScene() = default;

    [[nodiscard]] virtual bool AddPrimitive(const PrimitiveComponent* primitiveComponent,
                                            FPrimitiveComponentId primitiveComponentId,
                                            std::unique_ptr<FPrimitiveSceneProxy> proxy) = 0;
    virtual void UpdatePrimitiveTransform(FPrimitiveComponentId primitiveComponentId, const Matrix4& worldMatrix) = 0;
    virtual void RemovePrimitive(FPrimitiveComponentId primitiveComponentId) = 0;

    [[nodiscard]] virtual bool AddLight(const LightComponent* lightComponent,
                                        FLightComponentId lightComponentId,
                                        std::unique_ptr<FLightSceneProxy> proxy) = 0;
    virtual void UpdateLight(FLightComponentId lightComponentId, std::unique_ptr<FLightSceneProxy> proxy) = 0;
    virtual void RemoveLight(FLightComponentId lightComponentId) = 0;
};

} // namespace TE
