// ToyEngine Scene Module
// TWorld - 游戏世界
// 对应 UE5 的 UWorld
//
// 拥有 Actor 列表，管理游戏侧数据的更新和与渲染侧的同步
// 核心职责：Tick() 更新逻辑 + SyncToScene() 同步到渲染场景接口

#pragma once

#include "Actor.h"
#include "RenderScene.h"
#include <vector>
#include <memory>

namespace TE {

class TPrimitiveComponent;

/// 游戏世界
///
/// UE5 映射：
/// - UWorld: 游戏世界容器
/// - 持有 AActor 列表
/// - Tick() 遍历 Actor 更新
///
/// ToyEngine 扩展：
/// - 维护已注册的 PrimitiveComponent 列表（用于 SyncToScene）
/// - SyncToScene() 遍历脏 Component 将数据同步到渲染场景接口
class TWorld
{
public:
    TWorld() = default;
    ~TWorld() = default;

    // 禁止拷贝
    TWorld(const TWorld&) = delete;
    TWorld& operator=(const TWorld&) = delete;

    /// 添加 Actor 到世界
    /// 添加后会遍历其 PrimitiveComponent 注册到 FScene
    TActor* AddActor(std::unique_ptr<TActor> actor);

    /// 创建 Actor（模板方法）
    template<typename T = TActor, typename... Args>
    [[nodiscard]] T* SpawnActor(Args&&... args)
    {
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = actor.get();
        AddActor(std::move(actor));
        return ptr;
    }

    /// 每帧逻辑更新（遍历所有 Actor 的 Tick）
    void Tick(float deltaTime);

    /// 将脏 Component 的数据同步到渲染场景接口
    /// 单线程版本：直接赋值（因为同一线程安全）
    /// 将来双线程：改为 Enqueue 命令到渲染线程
    void SyncToScene();

    /// 注册/反注册 PrimitiveComponent（由组件注册流程调用）
    void RegisterPrimitiveComponent(TPrimitiveComponent* comp);
    void UnregisterPrimitiveComponent(TPrimitiveComponent* comp);

    /// 设置渲染场景接口（AddActor 时用于自动注册渲染对象）
    void SetRenderScene(IRenderScene* renderScene) { m_RenderScene = renderScene; }

    /// 获取所有 Actor
    [[nodiscard]] const std::vector<std::unique_ptr<TActor>>& GetActors() const { return m_Actors; }

private:
    std::vector<std::unique_ptr<TActor>>    m_Actors;
    std::vector<TPrimitiveComponent*>       m_PrimitiveComponents;  // 所有已注册的可渲染组件
    IRenderScene* m_RenderScene = nullptr; // 渲染场景接口引用
};

} // namespace TE
