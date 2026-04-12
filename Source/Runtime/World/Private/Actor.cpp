// ToyEngine Scene Module
// TActor 实现

#include "Actor.h"
#include "SceneComponent.h"
#include "Log/Log.h"

namespace TE {

Transform Actor::s_DefaultTransform;

void Actor::Tick(float deltaTime)
{
    // 遍历所有组件调用 Tick
    for (auto& comp : m_Components)
    {
        comp->Tick(deltaTime);
    }
}

Transform& Actor::GetTransform()
{
    if (m_RootComponent)
        return m_RootComponent->GetTransform();
    return s_DefaultTransform;
}

const Transform& Actor::GetTransform() const
{
    if (m_RootComponent)
        return m_RootComponent->GetTransform();
    return s_DefaultTransform;
}

void Actor::SetPosition(const Vector3& pos) const {
    if (m_RootComponent)
        m_RootComponent->SetPosition(pos);
}

Vector3 Actor::GetPosition() const
{
    if (m_RootComponent)
        return m_RootComponent->GetPosition();
    return Vector3::Zero;
}

} // namespace TE
