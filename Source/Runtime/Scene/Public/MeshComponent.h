// ToyEngine Scene Module
// TMeshComponent - 网格组件
// 对应 UE5 的 UStaticMeshComponent
//
// 持有网格数据描述（顶点、索引、Shader 路径等），
// override CreateSceneProxy() 创建 FStaticMeshSceneProxy

#pragma once

#include "PrimitiveComponent.h"
#include "FStaticMeshSceneProxy.h"
#include <vector>
#include <string>

namespace TE {

/// 网格组件
///
/// UE5 映射：
/// - UStaticMeshComponent: 引用 UStaticMesh 资源
/// - CreateSceneProxy() 返回 FStaticMeshSceneProxy
///
/// ToyEngine 简化版：
/// - 直接持有顶点/索引数据（而非引用独立的 Mesh 资产）
/// - SetMeshData() 设置网格数据
/// - CreateSceneProxy() 将数据传给 FStaticMeshSceneProxy 构造
class TMeshComponent : public TPrimitiveComponent
{
public:
    TMeshComponent() = default;
    ~TMeshComponent() override = default;

    /// 设置网格数据
    void SetMeshData(const FStaticMeshData& meshData) { m_MeshData = meshData; }
    [[nodiscard]] const FStaticMeshData& GetMeshData() const { return m_MeshData; }

    /// 创建渲染侧 Proxy（override）
    FPrimitiveSceneProxy* CreateSceneProxy(RHIDevice* device) override;

private:
    FStaticMeshData m_MeshData;
};

} // namespace TE
