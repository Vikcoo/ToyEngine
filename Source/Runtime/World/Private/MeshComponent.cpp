// ToyEngine Scene Module
// TMeshComponent 实现

#include "MeshComponent.h"
#include "FStaticMeshSceneProxy.h"
#include "Log/Log.h"
#include "RHIPipeline.h"
#include "TStaticMesh.h"

namespace TE {

std::unique_ptr<FPrimitiveSceneProxy> TMeshComponent::CreateSceneProxy(IRenderScene& renderScene) const
{
    if (!m_StaticMesh || !m_StaticMesh->IsValid())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy called with invalid static mesh");
        return nullptr;
    }

    auto renderData = renderScene.GetStaticMeshRenderData(m_StaticMesh);
    if (!renderData || !renderData->IsValid())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy failed to get static mesh render data");
        return nullptr;
    }

    auto* pipeline = renderScene.GetStaticMeshPipeline();
    if (!pipeline || !pipeline->IsValid())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy failed to get static mesh pipeline");
        return nullptr;
    }

    auto proxy = std::make_unique<FStaticMeshSceneProxy>(std::move(renderData), pipeline);
    if (!proxy->IsValid())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::CreateSceneProxy created invalid scene proxy");
        return nullptr;
    }

    return proxy;
}

} // namespace TE
