// ToyEngine Renderer Module
// RendererBindingSlots - 当前主渲染路径使用的 BindGroup 与资源 binding 约定

#pragma once

#include <cstdint>

namespace TE::RendererBindGroups {

constexpr uint32_t LightBlock = 0;
constexpr uint32_t PassBlock = 1;
constexpr uint32_t MaterialTextures = 2;
constexpr uint32_t GBufferTextures = 2;

} // namespace TE::RendererBindGroups

namespace TE::RendererBindings {

constexpr uint32_t LightBlock = 0;
constexpr uint32_t PassBlock = 1;

constexpr uint32_t BaseColorTexture = 2;

constexpr uint32_t GBufferAlbedo = 2;
constexpr uint32_t GBufferNormal = 3;
constexpr uint32_t GBufferWorldPosition = 4;
constexpr uint32_t GBufferDepth = 5;

} // namespace TE::RendererBindings
