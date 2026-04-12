// ToyEngine Scene Module
// TMeshComponent 实现

#include "MeshComponent.h"
#include "StaticMeshSceneProxy.h"
#include "Log/Log.h"
#include "StaticMesh.h"

namespace TE {

std::unique_ptr<FPrimitiveSceneProxy> MeshComponent::CreateSceneProxy() const
{
    if (!m_StaticMesh || !m_StaticMesh->IsValid())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy called with invalid static mesh");
        return nullptr;
    }

    auto proxy = std::make_unique<FStaticMeshSceneProxy>(m_StaticMesh);
    if (!proxy || !proxy->HasStaticMeshAsset())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy created invalid static mesh scene proxy");
        return nullptr;
    }

    return proxy;
}

} // namespace TE
