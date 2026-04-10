// ToyEngine Scene Module
// TPrimitiveComponent 实现
// 核心桥接：RegisterToRenderScene / UnregisterFromRenderScene / MarkRenderStateDirty

#include "PrimitiveComponent.h"
#include "Log/Log.h"

namespace TE {

TPrimitiveComponent::~TPrimitiveComponent()
{
    // 如果组件析构时仍处于注册状态，主动反注册，避免渲染侧残留句柄
    if (m_BoundRenderBridge && m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle)
    {
        m_BoundRenderBridge->DestroyPrimitive(m_RenderPrimitiveHandle);
    }
    m_BoundRenderBridge = nullptr;
    m_RenderPrimitiveHandle = InvalidRenderPrimitiveHandle;
}

void TPrimitiveComponent::RegisterToRenderScene(IRenderSceneBridge* renderBridge)
{
    if (!renderBridge)
    {
        TE_LOG_WARN("[Scene] RegisterToRenderScene called with null render bridge");
        return;
    }

    // 如果已有渲染对象，先注销
    if (m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle)
    {
        UnregisterFromRenderScene(renderBridge);
    }

    RenderPrimitiveCreateInfo createInfo;
    if (!BuildRenderCreateInfo(createInfo))
    {
        TE_LOG_WARN("[Scene] BuildRenderCreateInfo failed");
        return;
    }

    createInfo.WorldMatrix = GetWorldMatrix();
    m_RenderPrimitiveHandle = renderBridge->CreatePrimitive(createInfo);
    if (m_RenderPrimitiveHandle == InvalidRenderPrimitiveHandle)
    {
        TE_LOG_WARN("[Scene] Render bridge failed to create primitive");
        return;
    }

    m_BoundRenderBridge = renderBridge;
    m_RenderStateDirty = false;
    TE_LOG_INFO("[Scene] TPrimitiveComponent registered to render scene");
}

void TPrimitiveComponent::UnregisterFromRenderScene(IRenderSceneBridge* renderBridge)
{
    if (!renderBridge)
    {
        return;
    }
    if (m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle)
    {
        renderBridge->DestroyPrimitive(m_RenderPrimitiveHandle);
        m_RenderPrimitiveHandle = InvalidRenderPrimitiveHandle;
        m_BoundRenderBridge = nullptr;
        TE_LOG_INFO("[Scene] TPrimitiveComponent unregistered from render scene");
    }
}

} // namespace TE
