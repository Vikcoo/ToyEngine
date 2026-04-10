// ToyEngine Scene Module
// TMeshComponent 实现

#include "MeshComponent.h"
#include "TStaticMesh.h"
#include "Log/Log.h"

namespace TE {

bool TMeshComponent::BuildRenderCreateInfo(RenderPrimitiveCreateInfo& outCreateInfo) const
{
    if (!m_StaticMesh || !m_StaticMesh->IsValid())
    {
        TE_LOG_WARN("[Scene] TMeshComponent::BuildRenderCreateInfo called with invalid static mesh");
        return false;
    }

    outCreateInfo.Kind = RenderPrimitiveKind::StaticMesh;
    outCreateInfo.StaticMesh = m_StaticMesh;
    outCreateInfo.WorldMatrix = GetWorldMatrix();
    return true;
}

} // namespace TE
