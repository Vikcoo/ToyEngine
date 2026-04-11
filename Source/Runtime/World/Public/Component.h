// ToyEngine Scene Module
// TComponent - 组件基类
// 对应 UE5 的 UActorComponent
//
// 最基础的组件，提供 Owner 指针和虚方法 Tick()
// 所有组件（SceneComponent、PrimitiveComponent 等）都继承自此类

#pragma once

#include <string>

namespace TE {

class TActor;  // 前向声明

/// 组件基类
///
/// UE5 映射：
/// - UActorComponent: 所有组件的基类
/// - 提供 BeginPlay()、Tick()、EndPlay() 生命周期
///
/// ToyEngine 简化版：只保留 Tick() 和 Owner 关系
class TComponent
{
public:
    TComponent() = default;
    virtual ~TComponent() = default;

    // 禁止拷贝
    TComponent(const TComponent&) = delete;
    TComponent& operator=(const TComponent&) = delete;

    /// 每帧更新（子类 override）
    virtual void Tick(float deltaTime) {}

    /// 获取/设置所属 Actor
    void SetOwner(TActor* owner) { m_Owner = owner; }
    [[nodiscard]] TActor* GetOwner() const { return m_Owner; }

    /// 调试名称
    void SetName(const std::string& name) { m_Name = name; }
    [[nodiscard]] const std::string& GetName() const { return m_Name; }

protected:
    TActor*     m_Owner = nullptr;
    std::string m_Name;
};

} // namespace TE
