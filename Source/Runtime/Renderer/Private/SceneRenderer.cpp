// ToyEngine Renderer Module
// FSceneRenderer 实现

#include "SceneRenderer.h"

#include "DeferredRenderPath.h"
#include "ForwardRenderPath.h"
#include "IRenderPath.h"
#include "RenderStats.h"
#include "Log/Log.h"

namespace TE {

FSceneRenderer::FSceneRenderer()
    : m_RenderPath(CreateRenderPath(ERenderPathType::Forward))
{
}

FSceneRenderer::~FSceneRenderer() = default;

std::unique_ptr<IRenderPath> FSceneRenderer::CreateRenderPath(ERenderPathType type)
{
    switch (type)
    {
    case ERenderPathType::Forward:
        return std::make_unique<FForwardRenderPath>();
    case ERenderPathType::Deferred:
        return std::make_unique<FDeferredRenderPath>();
    default:
        return std::make_unique<FForwardRenderPath>();
    }
}

void FSceneRenderer::SetRenderPath(ERenderPathType type)
{
    if (m_RenderPath && m_RenderPathType == type)
    {
        return;
    }

    m_RenderPath = CreateRenderPath(type);
    m_RenderPathType = type;
    TE_LOG_INFO("[Renderer] Render path switched to {}",
                type == ERenderPathType::Forward ? "Forward" : "Deferred");
}

void FSceneRenderer::Render(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf)
{
    m_LastStats = {};
    if (!scene || !device || !cmdBuf)
    {
        TE_LOG_ERROR("[Renderer] SceneRenderer::Render called with null scene, device or cmdBuf");
        return;
    }

    if (!m_RenderPath)
    {
        TE_LOG_ERROR("[Renderer] SceneRenderer::Render called without active render path");
        return;
    }

    m_RenderPath->Render(scene, device, cmdBuf, m_LastStats);
}

} // namespace TE
