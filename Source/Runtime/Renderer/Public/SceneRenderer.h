// ToyEngine Renderer Module
// FSceneRenderer - 渲染路径调度器

#pragma once

#include "RenderStats.h"

#include <cstdint>
#include <memory>

namespace TE {

class FScene;
class IRenderPath;
class RHICommandBuffer;
class RHIDevice;

/// 场景渲染调度器。
///
/// Stage 2 后，FSceneRenderer 不再直接持有具体 Pass 顺序，只负责调用当前
/// IRenderPath。Stage 4 当前默认路径是 FDeferredRenderPath；运行时 Forward /
/// Deferred 切换入口留到 Stage 5。
class FSceneRenderer
{
public:
    FSceneRenderer();
    ~FSceneRenderer();

    FSceneRenderer(const FSceneRenderer&) = delete;
    FSceneRenderer& operator=(const FSceneRenderer&) = delete;

    void Render(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf);

    [[nodiscard]] uint32_t GetLastDrawCallCount() const { return m_LastStats.DrawCallCount; }
    [[nodiscard]] uint32_t GetLastPipelineBindCount() const { return m_LastStats.PipelineBindCount; }
    [[nodiscard]] uint32_t GetLastVBOBindCount() const { return m_LastStats.VBOBindCount; }
    [[nodiscard]] uint32_t GetLastIBOBindCount() const { return m_LastStats.IBOBindCount; }

private:
    std::unique_ptr<IRenderPath> m_RenderPath;
    FRenderStats m_LastStats;
};

} // namespace TE
