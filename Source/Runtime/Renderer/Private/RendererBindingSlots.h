// ToyEngine Renderer Module
// RendererBindingSlots - 当前主渲染路径使用的资源绑定槽位约定

#pragma once

#include <cstdint>

namespace TE::RendererBindingSlots {

constexpr uint32_t LightBlock = 0;
constexpr uint32_t PassBlock = 1;

constexpr uint32_t BaseColorTexture = 2;

constexpr uint32_t GBufferAlbedo = 2;
constexpr uint32_t GBufferNormal = 3;
constexpr uint32_t GBufferWorldPosition = 4;
constexpr uint32_t GBufferDepth = 5;

} // namespace TE::RendererBindingSlots
