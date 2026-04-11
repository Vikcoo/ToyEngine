// ToyEngine Scene Module
// TPrimitiveComponent 实现
// 核心同步：RegisterToRenderScene / UnregisterFromRenderScene / MarkRenderStateDirty

#include "PrimitiveComponent.h"
#include "Log/Log.h"

namespace TE {

TPrimitiveComponent::~TPrimitiveComponent()
{
    // 如果组件析构时仍处于注册状态，主动反注册，避免渲染侧残留句柄
    if (m_BoundRenderScene && m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle)
    {
        m_BoundRenderScene->DestroyPrimitive(m_RenderPrimitiveHandle);
    }
    m_BoundRenderScene = nullptr;
    m_RenderPrimitiveHandle = InvalidRenderPrimitiveHandle;
}

void TPrimitiveComponent::RegisterToRenderScene(IRenderScene* renderScene)
{
    if (!renderScene)
    {
        TE_LOG_WARN("[Scene] RegisterToRenderScene called with null render scene");
        return;
    }

    // 如果已有渲染对象，先注销
    if (m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle)
    {
        UnregisterFromRenderScene(renderScene);
    }

    RenderPrimitiveCreateInfo createInfo;
    if (!BuildRenderCreateInfo(createInfo))
    {
        TE_LOG_WARN("[Scene] BuildRenderCreateInfo failed");
        return;
    }

    createInfo.WorldMatrix = GetWorldMatrix();
    m_RenderPrimitiveHandle = renderScene->CreatePrimitive(createInfo);
    if (m_RenderPrimitiveHandle == InvalidRenderPrimitiveHandle)
    {
        TE_LOG_WARN("[Scene] Render scene failed to create primitive");
        return;
    }

    m_BoundRenderScene = renderScene;
    m_RenderStateDirty = false;
    TE_LOG_INFO("[Scene] TPrimitiveComponent registered to render scene");
}

void TPrimitiveComponent::UnregisterFromRenderScene(IRenderScene* renderScene)
{
    if (!renderScene)
    {
        return;
    }
    if (m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle)
    {
        renderScene->DestroyPrimitive(m_RenderPrimitiveHandle);
        m_RenderPrimitiveHandle = InvalidRenderPrimitiveHandle;
        m_BoundRenderScene = nullptr;
        TE_LOG_INFO("[Scene] TPrimitiveComponent unregistered from render scene");
    }
}

} // namespace TE
