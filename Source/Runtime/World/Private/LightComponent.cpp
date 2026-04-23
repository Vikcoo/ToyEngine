// ToyEngine World Module
// LightComponent 实现

#include "LightComponent.h"

#include "Log/Log.h"

#include <atomic>

namespace TE {

namespace {
FLightComponentId AllocateLightComponentId()
{
    static std::atomic<uint32_t> nextId{1};

    FLightComponentId lightComponentId;
    lightComponentId.Value = nextId.fetch_add(1, std::memory_order_relaxed);
    return lightComponentId;
}
} // namespace

LightComponent::LightComponent()
    : m_LightComponentId(AllocateLightComponentId())
{
}

LightComponent::~LightComponent()
{
    if (m_BoundRenderScene && m_IsRegisteredToRenderScene)
    {
        m_BoundRenderScene->RemoveLight(m_LightComponentId);
    }
    m_BoundRenderScene = nullptr;
    m_IsRegisteredToRenderScene = false;
}

std::unique_ptr<FLightSceneProxy> LightComponent::CreateLightSceneProxy() const
{
    auto proxy = std::make_unique<FLightSceneProxy>();
    proxy->Color = m_Color;
    proxy->Intensity = m_Intensity;
    proxy->Direction = GetWorldForward();
    proxy->Position = GetPosition();
    return proxy;
}

void LightComponent::RegisterToRenderScene(IRenderScene* renderScene)
{
    if (!renderScene)
    {
        TE_LOG_WARN("[Scene] LightComponent::RegisterToRenderScene called with null render scene");
        return;
    }

    if (m_IsRegisteredToRenderScene)
    {
        UnregisterFromRenderScene(renderScene);
    }

    auto proxy = CreateLightSceneProxy();
    if (!proxy)
    {
        TE_LOG_WARN("[Scene] LightComponent::CreateLightSceneProxy failed");
        return;
    }

    if (!renderScene->AddLight(this, m_LightComponentId, std::move(proxy)))
    {
        TE_LOG_WARN("[Scene] Render scene failed to add light");
        return;
    }

    m_BoundRenderScene = renderScene;
    m_IsRegisteredToRenderScene = true;
    m_LightStateDirty = false;
    TE_LOG_INFO("[Scene] LightComponent registered to render scene");
}

void LightComponent::UnregisterFromRenderScene(IRenderScene* renderScene)
{
    if (!renderScene)
    {
        return;
    }

    if (m_IsRegisteredToRenderScene)
    {
        renderScene->RemoveLight(m_LightComponentId);
        m_IsRegisteredToRenderScene = false;
        m_BoundRenderScene = nullptr;
        TE_LOG_INFO("[Scene] LightComponent unregistered from render scene");
    }
}

Vector3 LightComponent::GetWorldForward() const
{
    const Vector3 forward = GetTransform().GetForward();
    const Vector3 normalized = forward.Normalize();
    return normalized.LengthSquared() > 0.0f ? normalized : Vector3::Forward;
}

std::unique_ptr<FLightSceneProxy> DirectionalLightComponent::CreateLightSceneProxy() const
{
    auto proxy = LightComponent::CreateLightSceneProxy();
    proxy->Type = ELightType::Directional;
    return proxy;
}

std::unique_ptr<FLightSceneProxy> PointLightComponent::CreateLightSceneProxy() const
{
    auto proxy = LightComponent::CreateLightSceneProxy();
    proxy->Type = ELightType::Point;
    proxy->AttenuationRadius = m_AttenuationRadius;
    return proxy;
}

} // namespace TE
