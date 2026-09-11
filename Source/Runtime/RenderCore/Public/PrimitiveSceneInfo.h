// ToyEngine RenderCore Module
// FPrimitiveSceneInfo - Primitive 在场景注册后的运行时节点
// 对应 UE5 的 FPrimitiveSceneInfo（精简版）

#pragma once

#include "PrimitiveSceneProxy.h"
#include "PrimitiveComponentId.h"

#include <memory>

namespace TE {

class FPrimitiveSceneInfo
{
public:
    FPrimitiveSceneInfo(FPrimitiveComponentId primitiveComponentId,
                        std::unique_ptr<FPrimitiveSceneProxy> proxy)
        : m_PrimitiveComponentId(primitiveComponentId)
        , m_Proxy(std::move(proxy))
    {
    }

    [[nodiscard]] FPrimitiveComponentId GetPrimitiveComponentId() const { return m_PrimitiveComponentId; }
    [[nodiscard]] FPrimitiveSceneProxy* GetProxy() const { return m_Proxy.get(); }

private:
    FPrimitiveComponentId m_PrimitiveComponentId;
    std::unique_ptr<FPrimitiveSceneProxy> m_Proxy;
};

} // namespace TE
