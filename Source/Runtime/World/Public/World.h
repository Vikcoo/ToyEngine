// ToyEngine World Module
// World - 游戏侧对象容器

#pragma once

#include "Actor.h"
#include "RenderScene.h"

#include <memory>
#include <utility>
#include <vector>

namespace TE {

class LightComponent;
class PrimitiveComponent;

class World
{
public:
    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    Actor* AddActor(std::unique_ptr<Actor> actor);

    template<typename T = Actor, typename... Args>
    [[nodiscard]] T* SpawnActor(Args&&... args)
    {
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = actor.get();
        AddActor(std::move(actor));
        return ptr;
    }

    void Tick(float deltaTime);
    void SyncToScene();

    void RegisterPrimitiveComponent(PrimitiveComponent* comp);
    void UnregisterPrimitiveComponent(PrimitiveComponent* comp);
    void RegisterLightComponent(LightComponent* comp);
    void UnregisterLightComponent(LightComponent* comp);

    void SetRenderScene(IRenderScene* renderScene) { m_RenderScene = renderScene; }

    [[nodiscard]] const std::vector<std::unique_ptr<Actor>>& GetActors() const { return m_Actors; }

private:
    std::vector<std::unique_ptr<Actor>> m_Actors;
    std::vector<PrimitiveComponent*> m_PrimitiveComponents;
    std::vector<LightComponent*> m_LightComponents;
    IRenderScene* m_RenderScene = nullptr;
};

} // namespace TE
