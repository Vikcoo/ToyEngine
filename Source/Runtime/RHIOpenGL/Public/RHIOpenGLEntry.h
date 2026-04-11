// ToyEngine RHIOpenGL Module
// RHIOpenGL 后端装配入口（供外层模块调用）

#pragma once

#include <memory>

namespace TE {

class RHIDevice;

/// 创建 RHIOpenGL 后端的 RHI Device。
/// 该函数用于将具体后端创建逻辑暴露给外层装配模块，避免 RHI 抽象层反向依赖后端实现。
[[nodiscard]] std::unique_ptr<RHIDevice> CreateRHIOpenGLDevice();

} // namespace TE
