// ToyEngine Scene Module
// TMeshComponent - 网格组件
// 对应 UE5 的 UStaticMeshComponent
//
// 重构说明：
// - 持有 shared_ptr<TStaticMesh> 资产引用（对应 UE5 中 UStaticMeshComponent 引用 UStaticMesh）
// - CreateSceneProxy() 从 TStaticMesh 构建 FStaticMeshSceneProxy

#pragma once

#include "PrimitiveComponent.h"
#include <memory>

namespace TE {

// 前向声明
class TStaticMesh;

/// 网格组件
///
/// UE5 映射：
/// - UStaticMeshComponent: 引用 UStaticMesh 资源
/// - CreateSceneProxy() 返回 FStaticMeshSceneProxy
///
/// ToyEngine 简化版：
/// - 持有 shared_ptr<TStaticMesh> 资产引用（多个组件可共享同一资产）
/// - SetStaticMesh() 设置资产引用
/// - CreateSceneProxy() 将 TStaticMesh 传给 FStaticMeshSceneProxy 构造
class TMeshComponent : public TPrimitiveComponent
{
public:
    TMeshComponent() = default;
    ~TMeshComponent() override = default;

    /// 设置静态网格资产（对应 UE5 的 UStaticMeshComponent::SetStaticMesh）
    void SetStaticMesh(std::shared_ptr<TStaticMesh> mesh) { m_StaticMesh = std::move(mesh); }

    /// 获取静态网格资产引用
    [[nodiscard]] const std::shared_ptr<TStaticMesh>& GetStaticMesh() const { return m_StaticMesh; }

    /// 创建渲染侧 Proxy（override）
    [[nodiscard]] FPrimitiveSceneProxy* CreateSceneProxy(RHIDevice* device) override;

private:
    std::shared_ptr<TStaticMesh> m_StaticMesh;
};

} // namespace TE
