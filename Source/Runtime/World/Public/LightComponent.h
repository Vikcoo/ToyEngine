// ToyEngine World Module
// LightComponent - 光源组件

#pragma once

#include "LightComponentId.h"
#include "LightSceneProxy.h"
#include "SceneComponent.h"

#include <memory>

namespace TE {

class FRenderSceneCommandRecorder;

class LightComponent : public SceneComponent
{
public:
    LightComponent();
    ~LightComponent() override = default;

    [[nodiscard]] virtual std::unique_ptr<FLightSceneProxy> CreateLightSceneProxy() const;

    void MarkLightStateDirty() { m_LightStateDirty = true; }
    void ClearLightStateDirty() { m_LightStateDirty = false; }

    [[nodiscard]] bool IsLightStateDirty() const { return m_LightStateDirty; }
    [[nodiscard]] bool IsRenderStateCreated() const { return m_IsRenderStateCreated; }
    [[nodiscard]] FLightComponentId GetLightComponentId() const { return m_LightComponentId; }

    void SetColor(const Vector3& color) { m_Color = color; MarkLightStateDirty(); }
    [[nodiscard]] const Vector3& GetColor() const { return m_Color; }

    void SetIntensity(float intensity) { m_Intensity = intensity; MarkLightStateDirty(); }
    [[nodiscard]] float GetIntensity() const { return m_Intensity; }

protected:
    [[nodiscard]] Vector3 GetWorldForward() const;

    FLightComponentId m_LightComponentId;
    Vector3 m_Color = Vector3::One;
    float m_Intensity = 1.0f;
    bool m_IsRenderStateCreated = false;
    bool m_LightStateDirty = true;

private:
    friend class World;

    /** 为已注册到 World 的组件创建渲染状态。 */
    void CreateRenderState(FRenderSceneCommandRecorder& recorder);
    /** 销毁渲染状态，但不改变 Actor 对组件的所有权。 */
    void DestroyRenderState(FRenderSceneCommandRecorder& recorder);
};

class DirectionalLightComponent final : public LightComponent
{
public:
    DirectionalLightComponent() = default;
    ~DirectionalLightComponent() override = default;

    [[nodiscard]] std::unique_ptr<FLightSceneProxy> CreateLightSceneProxy() const override;
};

class PointLightComponent final : public LightComponent
{
public:
    PointLightComponent() = default;
    ~PointLightComponent() override = default;

    void SetAttenuationRadius(float radius) { m_AttenuationRadius = radius; MarkLightStateDirty(); }
    [[nodiscard]] float GetAttenuationRadius() const { return m_AttenuationRadius; }

    [[nodiscard]] std::unique_ptr<FLightSceneProxy> CreateLightSceneProxy() const override;

private:
    float m_AttenuationRadius = 10.0f;
};

} // namespace TE
