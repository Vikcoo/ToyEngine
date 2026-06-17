// ToyEngine Renderer Module
// RendererShaderNames - Renderer 使用的逻辑 Shader 名称

#pragma once

namespace TE::RendererShaderNames {

inline constexpr const char* StaticMeshBasePassVS = "StaticMesh/BasePassVS";
inline constexpr const char* StaticMeshBasePassPS = "StaticMesh/BasePassPS";
inline constexpr const char* StaticMeshGBufferVS = "StaticMesh/GBufferVS";
inline constexpr const char* StaticMeshGBufferPS = "StaticMesh/GBufferPS";
inline constexpr const char* DeferredLightingVS = "Deferred/LightingVS";
inline constexpr const char* DeferredLightingPS = "Deferred/LightingPS";
inline constexpr const char* SkyVS = "Sky/FullscreenVS";
inline constexpr const char* SkyPS = "Sky/SkyPS";

} // namespace TE::RendererShaderNames
