// ToyEngine Renderer Module
// 默认渲染桥接实现

#include "RenderSceneBridgeFactory.h"

#include "FScene.h"
#include "FStaticMeshSceneProxy.h"
#include "RHIDevice.h"
#include "TStaticMesh.h"
#include "Log/Log.h"

namespace TE {

class RenderSceneBridge final : public IRenderSceneBridge
{
public:
    RenderSceneBridge(FScene* scene, RHIDevice* device)
        : m_Scene(scene)
        , m_Device(device)
    {
    }

    [[nodiscard]] RenderPrimitiveHandle CreatePrimitive(const RenderPrimitiveCreateInfo& createInfo) override
    {
        if (!m_Scene || !m_Device)
        {
            TE_LOG_ERROR("[Renderer] RenderSceneBridge invalid scene/device");
            return InvalidRenderPrimitiveHandle;
        }

        switch (createInfo.Kind)
        {
        case RenderPrimitiveKind::StaticMesh:
        {
            if (!createInfo.StaticMesh || !createInfo.StaticMesh->IsValid())
            {
                TE_LOG_WARN("[Renderer] RenderSceneBridge static mesh create info is invalid");
                return InvalidRenderPrimitiveHandle;
            }

            auto proxy = std::make_unique<FStaticMeshSceneProxy>(createInfo.StaticMesh.get(), m_Device);
            if (!proxy->IsValid())
            {
                TE_LOG_WARN("[Renderer] RenderSceneBridge failed to create valid static mesh proxy");
                return InvalidRenderPrimitiveHandle;
            }

            proxy->SetWorldMatrix(createInfo.WorldMatrix);
            return m_Scene->AddPrimitive(std::move(proxy));
        }
        default:
            TE_LOG_WARN("[Renderer] RenderSceneBridge unsupported primitive kind");
            return InvalidRenderPrimitiveHandle;
        }
    }

    void UpdatePrimitiveTransform(RenderPrimitiveHandle handle, const Matrix4& worldMatrix) override
    {
        if (!m_Scene || handle == InvalidRenderPrimitiveHandle)
        {
            return;
        }
        m_Scene->UpdatePrimitiveWorldMatrix(handle, worldMatrix);
    }

    void DestroyPrimitive(RenderPrimitiveHandle handle) override
    {
        if (!m_Scene || handle == InvalidRenderPrimitiveHandle)
        {
            return;
        }
        m_Scene->RemovePrimitive(handle);
    }

private:
    FScene* m_Scene = nullptr;
    RHIDevice* m_Device = nullptr;
};

std::unique_ptr<IRenderSceneBridge> CreateRenderSceneBridge(FScene* scene, RHIDevice* device)
{
    return std::make_unique<RenderSceneBridge>(scene, device);
}

} // namespace TE
