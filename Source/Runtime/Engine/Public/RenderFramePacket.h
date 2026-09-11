#pragma once

#include "RenderPathTypes.h"
#include "RenderSceneCommand.h"
#include "SceneViewInfo.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace TE {

/** 游戏线程生成、渲染阶段消费的一帧完整输入快照。 */
struct FRenderFramePacket
{
    uint64_t FrameNumber = 0;
    std::vector<FRenderSceneCommand> SceneCommands;
    std::optional<FViewInfo> View;
    uint32_t FramebufferWidth = 0;
    uint32_t FramebufferHeight = 0;
    ERenderPathType RenderPath = ERenderPathType::Forward;
    ERenderDebugView DebugView = ERenderDebugView::Lit;
    bool bVSync = true;
};

} // namespace TE
