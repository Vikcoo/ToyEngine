// ToyEngine Scene Module
// TMeshComponent - 网格组件
// 对应 UE5 的 UStaticMeshComponent
//
// 重构说明：
// - 持有 shared_ptr<TStaticMesh> 资产引用（对应 UE5 中 UStaticMeshComponent 引用 UStaticMesh）
// - 通过 BuildRenderCreateInfo() 提供渲染创建数据，具体渲染对象由渲染场景接口创建

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
/// - 通过渲染场景接口将网格数据映射到渲染侧镜像
///
/// ToyEngine 简化版：
/// - 持有 shared_ptr<TStaticMesh> 资产引用（多个组件可共享同一资产）
/// - SetStaticMesh() 设置资产引用
/// - BuildRenderCreateInfo() 填充渲染创建信息
class TMeshComponent : public TPrimitiveComponent
{
public:
    TMeshComponent() = default;
    ~TMeshComponent() override = default;

    /// 设置静态网格资产（对应 UE5 的 UStaticMeshComponent::SetStaticMesh）
    void SetStaticMesh(std::shared_ptr<TStaticMesh> mesh) { m_StaticMesh = std::move(mesh); }

    /// 获取静态网格资产引用
    [[nodiscard]] const std::shared_ptr<TStaticMesh>& GetStaticMesh() const { return m_StaticMesh; }

    /// 填充渲染创建信息（override）
    [[nodiscard]] bool BuildRenderCreateInfo(RenderPrimitiveCreateInfo& outCreateInfo) const override;

private:
    std::shared_ptr<TStaticMesh> m_StaticMesh;
};

} // namespace TE
