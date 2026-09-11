// ToyEngine Scene Module
// TPrimitiveComponent 实现
// 核心同步：CreateRenderState / DestroyRenderState / MarkRenderStateDirty

#include "PrimitiveComponent.h"
#include "Log/Log.h"
#include "RenderSceneCommandRecorder.h"

#include <atomic>

namespace TE {

namespace {
FPrimitiveComponentId AllocatePrimitiveComponentId()
{
    static std::atomic<uint32_t> nextId{1};

    FPrimitiveComponentId primitiveComponentId;
    primitiveComponentId.Value = nextId.fetch_add(1, std::memory_order_relaxed);
    return primitiveComponentId;
}
} // namespace

PrimitiveComponent::PrimitiveComponent()
    : m_PrimitiveComponentId(AllocatePrimitiveComponentId())
{
}

void PrimitiveComponent::CreateRenderState(FRenderSceneCommandRecorder& recorder)
{
    if (m_IsRenderStateCreated)
    {
        return;
    }

    auto proxy = CreateSceneProxy();
    if (!proxy)
    {
        TE_LOG_WARN("[Scene] CreateSceneProxy failed");
        return;
    }

    proxy->SetWorldMatrix(GetWorldMatrix());
    if (!recorder.AddPrimitive(m_PrimitiveComponentId, std::move(proxy)))
    {
        TE_LOG_WARN("[Scene] Failed to record primitive add command");
        return;
    }

    m_IsRenderStateCreated = true;
    m_RenderStateDirty = false;
    TE_LOG_INFO("[Scene] TPrimitiveComponent registered to render scene");
}

void PrimitiveComponent::DestroyRenderState(FRenderSceneCommandRecorder& recorder)
{
    if (m_IsRenderStateCreated)
    {
        recorder.RemovePrimitive(m_PrimitiveComponentId);
        m_IsRenderStateCreated = false;
        TE_LOG_INFO("[Scene] TPrimitiveComponent render state destroyed");
    }
}

} // namespace TE
