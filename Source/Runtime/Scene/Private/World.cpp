// ToyEngine Scene Module
// TWorld 实现
// 核心：Tick() 更新逻辑 + SyncToScene() 同步到渲染侧

#include "World.h"
#include "PrimitiveComponent.h"
#include "FScene.h"
#include "FPrimitiveSceneProxy.h"
#include "Log/Log.h"
#include <algorithm>

namespace TE {

TActor* TWorld::AddActor(std::unique_ptr<TActor> actor)
{
    if (!actor)
    {
        TE_LOG_WARN("[Scene] TWorld::AddActor called with null actor");
        return nullptr;
    }

    TActor* ptr = actor.get();
    m_Actors.push_back(std::move(actor));

    // 遍历 Actor 的所有组件，注册 PrimitiveComponent 到 FScene
    for (const auto& comp : ptr->GetComponents())
    {
        if (auto* primComp = dynamic_cast<TPrimitiveComponent*>(comp.get()))
        {
            RegisterPrimitiveComponent(primComp);

            // 如果有 FScene 和 RHIDevice，自动注册到渲染场景
            if (m_Scene && m_RHIDevice)
            {
                primComp->RegisterToScene(m_Scene, m_RHIDevice);
            }
        }
    }

    TE_LOG_INFO("[Scene] TWorld::AddActor '{}', total actors: {}",
                ptr->GetName(), m_Actors.size());
    return ptr;
}

void TWorld::Tick(float deltaTime)
{
    // 遍历所有 Actor 更新逻辑
    for (auto& actor : m_Actors)
    {
        actor->Tick(deltaTime);
    }
}

void TWorld::SyncToScene(FScene* scene)
{
    if (!scene)
        return;

    // 遍历所有已注册的 PrimitiveComponent
    // 如果标记为脏，将 WorldMatrix 同步到 Proxy
    for (auto* comp : m_PrimitiveComponents)
    {
        if (comp->IsRenderStateDirty())
        {
            auto* proxy = comp->GetSceneProxy();
            if (proxy)
            {
                // 单线程版本：直接赋值（安全）
                // 将来双线程：改为 Enqueue 命令
                proxy->SetWorldMatrix(comp->GetWorldMatrix());
            }
            comp->ClearRenderStateDirty();
        }
    }
}

void TWorld::RegisterPrimitiveComponent(TPrimitiveComponent* comp)
{
    if (!comp) return;

    auto it = std::find(m_PrimitiveComponents.begin(), m_PrimitiveComponents.end(), comp);
    if (it == m_PrimitiveComponents.end())
    {
        m_PrimitiveComponents.push_back(comp);
    }
}

void TWorld::UnregisterPrimitiveComponent(TPrimitiveComponent* comp)
{
    auto it = std::find(m_PrimitiveComponents.begin(), m_PrimitiveComponents.end(), comp);
    if (it != m_PrimitiveComponents.end())
    {
        m_PrimitiveComponents.erase(it);
    }
}

} // namespace TE
