// ToyEngine Scene Module
// TActor - 实体
// 对应 UE5 的 AActor
//
// 拥有组件列表，是游戏世界中的基本实体单位
// 提供 AddComponent<T>() 模板方法方便添加组件

#pragma once

#include "Component.h"
#include "Math/Transform.h"
#include <vector>
#include <memory>
#include <string>

namespace TE {

class TSceneComponent;

/// 实体类
///
/// UE5 映射：
/// - AActor: 游戏世界中的实体
/// - 拥有一个 RootComponent（决定 Actor 的 Transform）
/// - 拥有多个 Component 列表
///
/// ToyEngine 简化版：
/// - 持有 Component 列表（vector of unique_ptr<TComponent>）
/// - 第一个 TSceneComponent 自动成为 RootComponent
/// - Tick() 遍历所有组件调用其 Tick()
class TActor
{
public:
    TActor() = default;
    virtual ~TActor() = default;

    // 禁止拷贝
    TActor(const TActor&) = delete;
    TActor& operator=(const TActor&) = delete;

    /// 添加组件（模板方法，返回 raw 指针便于后续操作）
    template<typename T, typename... Args>
    [[nodiscard]] T* AddComponent(Args&&... args)
    {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        ptr->SetOwner(this);

        // 如果是 SceneComponent 且没有 RootComponent，自动设为 Root
        if (!m_RootComponent)
        {
            if (auto* sceneComp = dynamic_cast<TSceneComponent*>(ptr))
            {
                m_RootComponent = sceneComp;
            }
        }

        m_Components.push_back(std::move(component));
        return ptr;
    }

    /// 每帧更新（遍历所有组件）
    virtual void Tick(float deltaTime);

    /// 获取所有组件
    [[nodiscard]] const std::vector<std::unique_ptr<TComponent>>& GetComponents() const { return m_Components; }

    /// 获取 RootComponent 的 Transform（Actor 的位置/旋转/缩放）
    [[nodiscard]] Transform& GetTransform();
    [[nodiscard]] const Transform& GetTransform() const;

    /// 位置快捷访问（委托给 RootComponent）
    void SetPosition(const Vector3& pos) const;
    [[nodiscard]] Vector3 GetPosition() const;

    /// 名称
    void SetName(const std::string& name) { m_Name = name; }
    [[nodiscard]] const std::string& GetName() const { return m_Name; }

private:
    std::vector<std::unique_ptr<TComponent>>    m_Components;
    TSceneComponent*                            m_RootComponent = nullptr;
    std::string                                 m_Name;

    // 当没有 RootComponent 时使用的默认 Transform
    static Transform s_DefaultTransform;
};

} // namespace TE
