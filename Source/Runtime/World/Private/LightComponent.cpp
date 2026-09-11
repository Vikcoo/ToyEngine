// ToyEngine World Module
// LightComponent 实现

#include "LightComponent.h"

#include "Log/Log.h"
#include "RenderSceneCommandRecorder.h"

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

std::unique_ptr<FLightSceneProxy> LightComponent::CreateLightSceneProxy() const
{
    auto proxy = std::make_unique<FLightSceneProxy>();
    proxy->Color = m_Color;
    proxy->Intensity = m_Intensity;
    proxy->Direction = GetWorldForward();
    proxy->Position = GetPosition();
    return proxy;
}

void LightComponent::CreateRenderState(FRenderSceneCommandRecorder& recorder)
{
    if (m_IsRenderStateCreated)
    {
        return;
    }

    auto proxy = CreateLightSceneProxy();
    if (!proxy)
    {
        TE_LOG_WARN("[Scene] LightComponent::CreateLightSceneProxy failed");
        return;
    }

    if (!recorder.AddLight(m_LightComponentId, std::move(proxy)))
    {
        TE_LOG_WARN("[Scene] Failed to record light add command");
        return;
    }

    m_IsRenderStateCreated = true;
    m_LightStateDirty = false;
    TE_LOG_INFO("[Scene] LightComponent registered to render scene");
}

void LightComponent::DestroyRenderState(FRenderSceneCommandRecorder& recorder)
{
    if (m_IsRenderStateCreated)
    {
        recorder.RemoveLight(m_LightComponentId);
        m_IsRenderStateCreated = false;
        TE_LOG_INFO("[Scene] LightComponent render state destroyed");
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
