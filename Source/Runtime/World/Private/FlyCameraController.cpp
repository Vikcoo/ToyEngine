// ToyEngine Scene Module
// TFlyCameraController 实现

#include "FlyCameraController.h"
#include "Actor.h"
#include "CameraComponent.h"
#include "SceneComponent.h"
#include "InputManager.h"
#include "InputKeys.h"
#include "Window.h"
#include "Math/ScalarMath.h"

namespace TE
{

void FlyCameraController::Tick(float deltaTime)
{
    if (!m_Input || deltaTime <= 0.0f)
    {
        return;
    }

    Transform* targetTransform = FindTargetTransform();
    if (!targetTransform)
    {
        return;
    }

    if (!m_Initialized)
    {
        InitializeFromCurrentTransform(*targetTransform);
        m_Initialized = true;
    }

    ProcessLook(deltaTime, *targetTransform);
    ProcessMovement(deltaTime, *targetTransform);
}

void FlyCameraController::InitializeFromCurrentTransform(Transform& transform)
{
    const Vector3 forward = (-transform.GetForward()).Normalize();
    m_Pitch = Math::Asin(Math::Clamp(forward.Y, -1.0f, 1.0f));
    m_Yaw = Math::Atan2(forward.X, forward.Z);
}

Transform* FlyCameraController::FindTargetTransform() const
{
    Actor* owner = GetOwner();
    if (!owner)
    {
        return nullptr;
    }

    for (const auto& component : owner->GetComponents())
    {
        if (auto* cameraComponent = dynamic_cast<CameraComponent*>(component.get()))
        {
            return &cameraComponent->GetTransform();
        }
    }

    for (const auto& component : owner->GetComponents())
    {
        if (auto* sceneComponent = dynamic_cast<SceneComponent*>(component.get()))
        {
            return &sceneComponent->GetTransform();
        }
    }

    return nullptr;
}

void FlyCameraController::ProcessLook(float deltaTime, Transform& transform)
{
    (void)deltaTime;

    if (m_Input->IsMouseButtonJustPressed(MouseButton::Right))
    {
        m_LookModeActive = true;
        // Skip current frame and next frame to absorb cursor-mode switch warp.
        m_SkipLookDeltaFrames = 2;
        if (m_Window)
        {
            m_Window->SetCursorMode(CursorMode::Disabled);
        }
    }

    if (m_Input->IsMouseButtonJustReleased(MouseButton::Right))
    {
        m_LookModeActive = false;
        m_SkipLookDeltaFrames = 0;
        if (m_Window)
        {
            m_Window->SetCursorMode(CursorMode::Normal);
        }
    }

    if (!m_LookModeActive)
    {
        return;
    }

    const Vector2 mouseDelta = m_Input->GetMouseDelta();
    if (m_SkipLookDeltaFrames > 0)
    {
        --m_SkipLookDeltaFrames;
        return;
    }

    // Guard against occasional outlier deltas from OS/GLFW cursor warping.
    constexpr float kMaxMouseDeltaPixels = 500.0f;
    if (mouseDelta.LengthSquared() > (kMaxMouseDeltaPixels * kMaxMouseDeltaPixels))
    {
        return;
    }

    const float lookScale = m_LookSensitivity * Math::DEG_TO_RAD;
    m_Yaw -= mouseDelta.X * lookScale;
    m_Pitch += mouseDelta.Y * lookScale;

    const float pitchLimit = Math::DegToRad(m_MaxPitchDegrees);
    m_Pitch = Math::Clamp(m_Pitch, -pitchLimit, pitchLimit);

    transform.Rotation = Quat::FromEuler(m_Yaw + Math::PI, -m_Pitch, 0.0f).Normalize();
}

void FlyCameraController::ProcessMovement(float deltaTime, Transform& transform)
{
    const Vector2 scrollDelta = m_Input->GetScrollDelta();
    if (scrollDelta.Y != 0.0f)
    {
        m_MoveSpeed *= Math::Pow(m_ScrollSpeedFactor, scrollDelta.Y);
        m_MoveSpeed = Math::Clamp(m_MoveSpeed, 0.01f, 500.0f);
    }

    Vector3 velocity = Vector3::Zero;
    const Vector3 forward = (-transform.GetForward()).Normalize();
    const Vector3 right = Vector3::Cross(forward, Vector3::Up).Normalize();
    const Vector3 up = Vector3::Up;

    if (m_Input->IsKeyDown(Keys::W)) velocity += forward;
    if (m_Input->IsKeyDown(Keys::S)) velocity -= forward;
    if (m_Input->IsKeyDown(Keys::D)) velocity += right;
    if (m_Input->IsKeyDown(Keys::A)) velocity -= right;
    if (m_Input->IsKeyDown(Keys::E)) velocity += up;
    if (m_Input->IsKeyDown(Keys::Q)) velocity -= up;

    if (velocity.LengthSquared() <= 0.0f)
    {
        return;
    }

    velocity = velocity.Normalize();

    float speed = m_MoveSpeed;
    if (m_Input->IsKeyDown(Keys::LeftShift))
    {
        speed *= m_FastMultiplier;
    }
    if (m_Input->IsKeyDown(Keys::LeftControl))
    {
        speed *= m_SlowMultiplier;
    }

    transform.Position += velocity * speed * deltaTime;
}

} // namespace TE
