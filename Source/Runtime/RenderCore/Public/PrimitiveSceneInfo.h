// ToyEngine RenderCore Module
// FPrimitiveSceneInfo - Primitive 在场景注册后的运行时节点
// 对应 UE5 的 FPrimitiveSceneInfo（精简版）

#pragma once

#include "PrimitiveSceneProxy.h"
#include "PrimitiveComponentId.h"

#include <memory>

namespace TE {

class TPrimitiveComponent;

class FPrimitiveSceneInfo
{
public:
    FPrimitiveSceneInfo(FPrimitiveComponentId primitiveComponentId,
                        const TPrimitiveComponent* primitiveComponent,
                        std::unique_ptr<FPrimitiveSceneProxy> proxy)
        : m_PrimitiveComponentId(primitiveComponentId)
        , m_PrimitiveComponent(primitiveComponent)
        , m_Proxy(std::move(proxy))
    {
    }

    [[nodiscard]] FPrimitiveComponentId GetPrimitiveComponentId() const { return m_PrimitiveComponentId; }
    [[nodiscard]] const TPrimitiveComponent* GetPrimitiveComponent() const { return m_PrimitiveComponent; }
    [[nodiscard]] FPrimitiveSceneProxy* GetProxy() const { return m_Proxy.get(); }

private:
    FPrimitiveComponentId m_PrimitiveComponentId;
    const TPrimitiveComponent* m_PrimitiveComponent = nullptr;
    std::unique_ptr<FPrimitiveSceneProxy> m_Proxy;
};

} // namespace TE
