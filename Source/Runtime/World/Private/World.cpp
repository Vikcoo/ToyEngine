// ToyEngine Scene Module
// TWorld 实现
// 核心：Tick() 更新逻辑 + SyncToScene() 同步到渲染侧

#include "World.h"
#include "LightComponent.h"
#include "PrimitiveComponent.h"
#include "RenderSceneCommandRecorder.h"
#include "Log/Log.h"
#include <algorithm>

namespace TE {

World::~World()
{
    SetRenderSceneCommandRecorder(nullptr);
    for (const auto& actor : m_Actors)
    {
        actor->SetWorld(nullptr);
    }
    m_PrimitiveComponents.clear();
    m_LightComponents.clear();
}

Actor* World::AddActor(std::unique_ptr<Actor> actor)
{
    if (!actor)
    {
        TE_LOG_WARN("[Scene] TWorld::AddActor called with null actor");
        return nullptr;
    }

    m_Actors.push_back(std::move(actor));
    Actor* const ptr = m_Actors.back().get();
    ptr->SetWorld(this);

    for (const auto& component : ptr->GetComponents())
    {
        RegisterComponent(component.get());
    }

    TE_LOG_INFO("[Scene] TWorld::AddActor '{}', total actors: {}",
                ptr->GetName(), m_Actors.size());
    return ptr;
}

bool World::RemoveActor(Actor* actor)
{
    const auto it = std::find_if(m_Actors.begin(), m_Actors.end(),
                                 [actor](const std::unique_ptr<Actor>& ownedActor)
                                 {
                                     return ownedActor.get() == actor;
                                 });
    if (it == m_Actors.end())
    {
        return false;
    }

    for (const auto& component : (*it)->GetComponents())
    {
        UnregisterComponent(component.get());
    }
    (*it)->SetWorld(nullptr);
    m_Actors.erase(it);
    return true;
}

void World::Tick(float deltaTime)
{
    // 遍历所有 Actor 更新逻辑
    for (auto& actor : m_Actors)
    {
        actor->Tick(deltaTime);
    }
}

void World::SyncToScene()
{
    if (!m_RenderSceneCommandRecorder)
        return;

    // 遍历所有已注册的 PrimitiveComponent
    // 如果标记为脏，将 WorldMatrix 记录为渲染场景命令。
    for (auto* comp : m_PrimitiveComponents)
    {
        if (comp->IsRenderStateDirty() && comp->IsRenderStateCreated())
        {
            m_RenderSceneCommandRecorder->UpdatePrimitiveTransform(comp->GetPrimitiveComponentId(),
                                                                    comp->GetWorldMatrix());
            comp->ClearRenderStateDirty();
        }
    }

    for (auto* comp : m_LightComponents)
    {
        if (comp->IsLightStateDirty() && comp->IsRenderStateCreated())
        {
            m_RenderSceneCommandRecorder->UpdateLight(comp->GetLightComponentId(),
                                                       comp->CreateLightSceneProxy());
            comp->ClearLightStateDirty();
        }
    }
}

void World::RegisterComponent(Component* component)
{
    if (!component)
    {
        return;
    }
    if (!component->GetOwner() || component->GetOwner()->GetWorld() != this)
    {
        TE_LOG_WARN("[Scene] World::RegisterComponent rejected a component not owned by this world");
        return;
    }

    if (auto* primitiveComponent = dynamic_cast<PrimitiveComponent*>(component))
    {
        RegisterPrimitiveComponent(primitiveComponent);
    }
    if (auto* lightComponent = dynamic_cast<LightComponent*>(component))
    {
        RegisterLightComponent(lightComponent);
    }
}

void World::UnregisterComponent(Component* component)
{
    if (!component)
    {
        return;
    }

    if (auto* primitiveComponent = dynamic_cast<PrimitiveComponent*>(component))
    {
        UnregisterPrimitiveComponent(primitiveComponent);
    }
    if (auto* lightComponent = dynamic_cast<LightComponent*>(component))
    {
        UnregisterLightComponent(lightComponent);
    }
}

void World::SetRenderSceneCommandRecorder(FRenderSceneCommandRecorder* recorder)
{
    if (m_RenderSceneCommandRecorder == recorder)
    {
        return;
    }

    if (m_RenderSceneCommandRecorder)
    {
        for (PrimitiveComponent* component : m_PrimitiveComponents)
        {
            component->DestroyRenderState(*m_RenderSceneCommandRecorder);
        }
        for (LightComponent* component : m_LightComponents)
        {
            component->DestroyRenderState(*m_RenderSceneCommandRecorder);
        }
    }

    m_RenderSceneCommandRecorder = recorder;
    if (m_RenderSceneCommandRecorder)
    {
        for (PrimitiveComponent* component : m_PrimitiveComponents)
        {
            component->CreateRenderState(*m_RenderSceneCommandRecorder);
        }
        for (LightComponent* component : m_LightComponents)
        {
            component->CreateRenderState(*m_RenderSceneCommandRecorder);
        }
    }
}

void World::RegisterPrimitiveComponent(PrimitiveComponent* component)
{
    if (!component) return;

    const auto it = std::find(m_PrimitiveComponents.begin(), m_PrimitiveComponents.end(), component);
    if (it == m_PrimitiveComponents.end())
    {
        m_PrimitiveComponents.push_back(component);
        if (m_RenderSceneCommandRecorder)
        {
            component->CreateRenderState(*m_RenderSceneCommandRecorder);
        }
    }
}

void World::UnregisterPrimitiveComponent(PrimitiveComponent* component)
{
    const auto it = std::find(m_PrimitiveComponents.begin(), m_PrimitiveComponents.end(), component);
    if (it != m_PrimitiveComponents.end())
    {
        if (m_RenderSceneCommandRecorder)
        {
            component->DestroyRenderState(*m_RenderSceneCommandRecorder);
        }
        m_PrimitiveComponents.erase(it);
    }
}

void World::RegisterLightComponent(LightComponent* component)
{
    if (!component) return;

    const auto it = std::find(m_LightComponents.begin(), m_LightComponents.end(), component);
    if (it == m_LightComponents.end())
    {
        m_LightComponents.push_back(component);
        if (m_RenderSceneCommandRecorder)
        {
            component->CreateRenderState(*m_RenderSceneCommandRecorder);
        }
    }
}

void World::UnregisterLightComponent(LightComponent* component)
{
    const auto it = std::find(m_LightComponents.begin(), m_LightComponents.end(), component);
    if (it != m_LightComponents.end())
    {
        if (m_RenderSceneCommandRecorder)
        {
            component->DestroyRenderState(*m_RenderSceneCommandRecorder);
        }
        m_LightComponents.erase(it);
    }
}

} // namespace TE
