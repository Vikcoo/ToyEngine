// ToyEngine Scene Module
// TFlyCameraController - 默认视口飞行相机控制器

#pragma once

#include "Component.h"
#include "Math/MathTypes.h"

namespace TE
{

class FInputManager;
class IWindow;
class Transform;

class FlyCameraController : public Component
{
public:
    FlyCameraController() = default;
    ~FlyCameraController() override = default;

    void Tick(float deltaTime) override;

    void SetInputManager(FInputManager* inputManager) { m_Input = inputManager; }
    void SetWindow(IWindow* window) { m_Window = window; }

    void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }
    void SetLookSensitivity(float sensitivity) { m_LookSensitivity = sensitivity; }
    void SetFastMultiplier(float multiplier) { m_FastMultiplier = multiplier; }
    void SetSlowMultiplier(float multiplier) { m_SlowMultiplier = multiplier; }
    void SetPitchClamp(float degrees) { m_MaxPitchDegrees = degrees; }

    [[nodiscard]] float GetMoveSpeed() const { return m_MoveSpeed; }
    [[nodiscard]] float GetLookSensitivity() const { return m_LookSensitivity; }

private:
    void InitializeFromCurrentTransform(Transform& transform);
    [[nodiscard]] Transform* FindTargetTransform() const;
    void ProcessLook(float deltaTime, Transform& transform);
    void ProcessMovement(float deltaTime, Transform& transform);

private:
    FInputManager* m_Input = nullptr;
    IWindow* m_Window = nullptr;

    float m_MoveSpeed = 5.0f;
    float m_LookSensitivity = 0.15f;
    float m_FastMultiplier = 3.0f;
    float m_SlowMultiplier = 0.3f;
    float m_MaxPitchDegrees = 89.0f;
    float m_ScrollSpeedFactor = 1.1f;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    bool m_Initialized = false;
    bool m_LookModeActive = false;
    int m_SkipLookDeltaFrames = 0;
};

} // namespace TE
