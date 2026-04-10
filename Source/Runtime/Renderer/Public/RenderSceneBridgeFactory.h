// ToyEngine Renderer Module
// 渲染桥接对象工厂入口

#pragma once

#include "RenderSceneBridge.h"

#include <memory>

namespace TE {

class FScene;
class RHIDevice;

/// 创建默认渲染桥接实现。
/// 由外层装配模块调用，向 Scene 提供与 Renderer 解耦后的同步接口。
[[nodiscard]] std::unique_ptr<IRenderSceneBridge> CreateRenderSceneBridge(FScene* scene, RHIDevice* device);

} // namespace TE
