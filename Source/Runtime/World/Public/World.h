// ToyEngine World Module
// World - 游戏侧对象容器

#pragma once

#include "Actor.h"

#include <memory>
#include <utility>
#include <vector>

namespace TE {

class Component;
class LightComponent;
class PrimitiveComponent;
class FRenderSceneCommandRecorder;

class World
{
public:
    World() = default;
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    Actor* AddActor(std::unique_ptr<Actor> actor);
    /** 从 World 注销 Actor 的全部组件后销毁 Actor。 */
    [[nodiscard]] bool RemoveActor(Actor* actor);

    template<typename T = Actor, typename... Args>
    [[nodiscard]] T* SpawnActor(Args&&... args)
    {
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        T* const ptr = actor.get();
        AddActor(std::move(actor));
        return ptr;
    }

    void Tick(float deltaTime);
    void SyncToScene();

    /** 将 Actor 拥有的组件注册到 World 运行时系统。 */
    void RegisterComponent(Component* component);
    /** 从 World 运行时系统注销组件，但不改变 Actor 的所有权。 */
    void UnregisterComponent(Component* component);

    /** 设置游戏线程使用的渲染场景命令收集器。 */
    void SetRenderSceneCommandRecorder(FRenderSceneCommandRecorder* recorder);

    [[nodiscard]] const std::vector<std::unique_ptr<Actor>>& GetActors() const { return m_Actors; }

private:
    void RegisterPrimitiveComponent(PrimitiveComponent* component);
    void UnregisterPrimitiveComponent(PrimitiveComponent* component);
    void RegisterLightComponent(LightComponent* component);
    void UnregisterLightComponent(LightComponent* component);

    std::vector<std::unique_ptr<Actor>> m_Actors;
    std::vector<PrimitiveComponent*> m_PrimitiveComponents;
    std::vector<LightComponent*> m_LightComponents;
    FRenderSceneCommandRecorder* m_RenderSceneCommandRecorder = nullptr;
};

} // namespace TE
