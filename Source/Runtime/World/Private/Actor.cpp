// ToyEngine Scene Module
// TActor 实现

#include "Actor.h"
#include "SceneComponent.h"
#include "World.h"
#include "Log/Log.h"

#include <algorithm>

namespace TE {

Transform Actor::s_DefaultTransform;

void Actor::AddOwnedComponent(std::unique_ptr<Component> component)
{
    Component* const ptr = component.get();
    ptr->SetOwner(this);

    if (!m_RootComponent)
    {
        m_RootComponent = dynamic_cast<SceneComponent*>(ptr);
    }

    m_Components.push_back(std::move(component));
    if (m_World)
    {
        m_World->RegisterComponent(ptr);
    }
}

bool Actor::RemoveComponent(Component* component)
{
    const auto it = std::find_if(m_Components.begin(), m_Components.end(),
                                 [component](const std::unique_ptr<Component>& ownedComponent)
                                 {
                                     return ownedComponent.get() == component;
                                 });
    if (it == m_Components.end())
    {
        return false;
    }

    if (m_World)
    {
        m_World->UnregisterComponent(component);
    }

    const bool removedRoot = m_RootComponent == component;
    component->SetOwner(nullptr);
    m_Components.erase(it);

    if (removedRoot)
    {
        m_RootComponent = nullptr;
        for (const auto& ownedComponent : m_Components)
        {
            if (auto* sceneComponent = dynamic_cast<SceneComponent*>(ownedComponent.get()))
            {
                m_RootComponent = sceneComponent;
                break;
            }
        }
    }
    return true;
}

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
