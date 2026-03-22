// ToyEngine Scene Module
// TMeshComponent 实现

#include "MeshComponent.h"
#include "FStaticMeshSceneProxy.h"
#include "Log/Log.h"

namespace TE {

FPrimitiveSceneProxy* TMeshComponent::CreateSceneProxy(RHIDevice* device)
{
    if (m_MeshData.Vertices.empty() || m_MeshData.Indices.empty())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy called with empty mesh data");
        return nullptr;
    }

    // 创建 FStaticMeshSceneProxy，将网格数据和 RHIDevice 传入
    // Proxy 在构造函数中通过 RHIDevice 创建 GPU 资源
    return new FStaticMeshSceneProxy(m_MeshData, device);
}

} // namespace TE
