// ToyEngine Renderer Module
// FSceneRenderer 实现

#include "SceneRenderer.h"

#include "ForwardRenderPath.h"
#include "IRenderPath.h"
#include "RenderStats.h"
#include "Log/Log.h"

namespace TE {

FSceneRenderer::FSceneRenderer()
    : m_RenderPath(std::make_unique<FForwardRenderPath>())
{
}

FSceneRenderer::~FSceneRenderer() = default;

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
