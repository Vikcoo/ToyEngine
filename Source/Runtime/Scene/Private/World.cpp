// ToyEngine Scene Module
// TWorld 实现
// 核心：Tick() 更新逻辑 + SyncToScene() 同步到渲染侧

#include "World.h"
#include "PrimitiveComponent.h"
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


    m_Actors.push_back(std::move(actor));
    TActor* ptr = m_Actors.back().get();
    // 遍历 Actor 的所有组件，注册 PrimitiveComponent 到渲染场景接口
    for (const auto& comp : ptr->GetComponents())
    {
        if (auto* primComp = dynamic_cast<TPrimitiveComponent*>(comp.get()))
        {
            RegisterPrimitiveComponent(primComp);

            // 如果有渲染场景对象，自动注册到渲染侧
            if (m_RenderScene)
            {
                primComp->RegisterToRenderScene(m_RenderScene);
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

void TWorld::SyncToScene()
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
            m_RenderScene->UpdatePrimitiveTransform(comp->GetRenderPrimitiveHandle(), comp->GetWorldMatrix());
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
