// ToyEngine Scene Module
// TMeshComponent 实现

#include "MeshComponent.h"
#include "TStaticMesh.h"
#include "FStaticMeshSceneProxy.h"
#include "Log/Log.h"

namespace TE {

FPrimitiveSceneProxy* TMeshComponent::CreateSceneProxy(RHIDevice* device)
{
    if (!m_StaticMesh || !m_StaticMesh->IsValid())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy called with invalid static mesh");
        return nullptr;
    }

    // 创建 FStaticMeshSceneProxy，传入 TStaticMesh 资产和 RHIDevice
    // Proxy 在构造函数中从 TStaticMesh 读取 Section 数据，创建 GPU 资源
    return new FStaticMeshSceneProxy(m_StaticMesh.get(), device);
}

} // namespace TE
