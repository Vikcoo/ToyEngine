// ToyEngine Scene Module
// TWorld 实现
// 核心：Tick() 更新逻辑 + SyncToScene() 同步到渲染侧

#include "World.h"
#include "LightComponent.h"
#include "PrimitiveComponent.h"
#include "Log/Log.h"
#include <algorithm>

namespace TE {

Actor* World::AddActor(std::unique_ptr<Actor> actor)
{
    if (!actor)
    {
        TE_LOG_WARN("[Scene] TWorld::AddActor called with null actor");
        return nullptr;
    }


    m_Actors.push_back(std::move(actor));
    Actor* ptr = m_Actors.back().get();
    // 遍历 Actor 的所有组件，注册 PrimitiveComponent 到渲染场景接口
    for (const auto& comp : ptr->GetComponents())
    {
        if (auto* primComp = dynamic_cast<PrimitiveComponent*>(comp.get()))
        {
            RegisterPrimitiveComponent(primComp);

            // 如果有渲染场景对象，自动注册到渲染侧
            if (m_RenderScene)
            {
                primComp->RegisterToRenderScene(m_RenderScene);
            }
        }

        if (auto* lightComp = dynamic_cast<LightComponent*>(comp.get()))
        {
            RegisterLightComponent(lightComp);
            if (m_RenderScene)
            {
                lightComp->RegisterToRenderScene(m_RenderScene);
            }
        }
    }

    TE_LOG_INFO("[Scene] TWorld::AddActor '{}', total actors: {}",
                ptr->GetName(), m_Actors.size());
    return ptr;
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
    if (!m_RenderScene)
        return;

    // 遍历所有已注册的 PrimitiveComponent
    // 如果标记为脏，将 WorldMatrix 同步到渲染场景接口
    for (auto* comp : m_PrimitiveComponents)
    {
        if (comp->IsRenderStateDirty() && comp->IsRegisteredToRenderScene())
        {
            // 单线程版本：直接更新（安全）
            // 将来双线程：改为 Enqueue 命令
            m_RenderScene->UpdatePrimitiveTransform(comp->GetPrimitiveComponentId(), comp->GetWorldMatrix());
            comp->ClearRenderStateDirty();
        }
    }

    for (auto* comp : m_LightComponents)
    {
        if (comp->IsLightStateDirty() && comp->IsRegisteredToRenderScene())
        {
            m_RenderScene->UpdateLight(comp->GetLightComponentId(), comp->CreateLightSceneProxy());
            comp->ClearLightStateDirty();
        }
    }
}

void World::RegisterPrimitiveComponent(PrimitiveComponent* comp)
{
    if (!comp) return;

    auto it = std::find(m_PrimitiveComponents.begin(), m_PrimitiveComponents.end(), comp);
    if (it == m_PrimitiveComponents.end())
    {
        m_PrimitiveComponents.push_back(comp);
    }
}

void World::UnregisterPrimitiveComponent(PrimitiveComponent* comp)
{
    auto it = std::find(m_PrimitiveComponents.begin(), m_PrimitiveComponents.end(), comp);
    if (it != m_PrimitiveComponents.end())
    {
        m_PrimitiveComponents.erase(it);
    }
}

void World::RegisterLightComponent(LightComponent* comp)
{
    if (!comp) return;

    auto it = std::find(m_LightComponents.begin(), m_LightComponents.end(), comp);
    if (it == m_LightComponents.end())
    {
        m_LightComponents.push_back(comp);
    }
}

void World::UnregisterLightComponent(LightComponent* comp)
{
    auto it = std::find(m_LightComponents.begin(), m_LightComponents.end(), comp);
    if (it != m_LightComponents.end())
    {
        m_LightComponents.erase(it);
    }
}

} // namespace TE
