// ToyEngine Scene Module
// TPrimitiveComponent 实现
// 核心同步：RegisterToRenderScene / UnregisterFromRenderScene / MarkRenderStateDirty

#include "PrimitiveComponent.h"
#include "Log/Log.h"

namespace TE {

TPrimitiveComponent::~TPrimitiveComponent()
{
    // 如果组件析构时仍处于注册状态，主动反注册，避免渲染侧残留对象
    if (m_BoundRenderScene && m_IsRegisteredToRenderScene)
    {
        m_BoundRenderScene->RemovePrimitive(this);
    }
    m_BoundRenderScene = nullptr;
    m_IsRegisteredToRenderScene = false;
}

void TPrimitiveComponent::RegisterToRenderScene(IRenderScene* renderScene)
{
    if (!renderScene)
    {
        TE_LOG_WARN("[Scene] RegisterToRenderScene called with null render scene");
        return;
    }

    // 如果已有渲染对象，先注销
    if (m_IsRegisteredToRenderScene)
    {
        UnregisterFromRenderScene(renderScene);
    }

    auto proxy = CreateSceneProxy(*renderScene);
    if (!proxy)
    {
        TE_LOG_WARN("[Scene] CreateSceneProxy failed");
        return;
    }

    proxy->SetWorldMatrix(GetWorldMatrix());
    if (!renderScene->AddPrimitive(this, std::move(proxy)))
    {
        TE_LOG_WARN("[Scene] Render scene failed to add primitive");
        return;
    }

    m_BoundRenderScene = renderScene;
    m_IsRegisteredToRenderScene = true;
    m_RenderStateDirty = false;
    TE_LOG_INFO("[Scene] TPrimitiveComponent registered to render scene");
}

void TPrimitiveComponent::UnregisterFromRenderScene(IRenderScene* renderScene)
{
    if (!renderScene)
    {
        return;
    }
    if (m_IsRegisteredToRenderScene)
    {
        renderScene->RemovePrimitive(this);
        m_IsRegisteredToRenderScene = false;
        m_BoundRenderScene = nullptr;
        TE_LOG_INFO("[Scene] TPrimitiveComponent unregistered from render scene");
    }
}

} // namespace TE
