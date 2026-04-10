// ToyEngine Scene Module
// TActor 实现

#include "Actor.h"
#include "SceneComponent.h"
#include "Log/Log.h"

namespace TE {

Transform TActor::s_DefaultTransform;

void TActor::Tick(float deltaTime)
{
    // 遍历所有组件调用 Tick
    for (auto& comp : m_Components)
    {
        comp->Tick(deltaTime);
    }
}

Transform& TActor::GetTransform()
{
    if (m_RootComponent)
        return m_RootComponent->GetTransform();
    return s_DefaultTransform;
}

const Transform& TActor::GetTransform() const
{
    if (m_RootComponent)
        return m_RootComponent->GetTransform();
    return s_DefaultTransform;
}

void TActor::SetPosition(const Vector3& pos) const {
    if (m_RootComponent)
        m_RootComponent->SetPosition(pos);
}

Vector3 TActor::GetPosition() const
{
    if (m_RootComponent)
        return m_RootComponent->GetPosition();
    return Vector3::Zero;
}

} // namespace TE
