// ToyEngine RHIOpenGL Module
// OpenGLShaderLibrary - OpenGL Shader 资产映射

#include "OpenGLShaderLibrary.h"

#include "Log/Log.h"

#include <string_view>

#ifndef TE_SHADER_ROOT_DIR
    #define TE_SHADER_ROOT_DIR ""
#endif

namespace TE {

namespace {

struct FOpenGLShaderAsset
{
    std::string_view LogicalName;
    std::string_view RelativePath;
};

constexpr FOpenGLShaderAsset ShaderAssets[] = {
    {"StaticMesh/BasePassVS", "model.vert"},
    {"StaticMesh/BasePassPS", "model.frag"},
    {"StaticMesh/GBufferVS", "gbuffer.vert"},
    {"StaticMesh/GBufferPS", "gbuffer.frag"},
    {"Deferred/LightingVS", "deferred_lighting.vert"},
    {"Deferred/LightingPS", "deferred_lighting.frag"},
};

} // namespace

std::string ResolveOpenGLShaderPath(const std::string& logicalName)
{
    for (const auto& asset : ShaderAssets)
    {
        if (asset.LogicalName == logicalName)
        {
            return std::string(TE_SHADER_ROOT_DIR) + "OpenGL/" + std::string(asset.RelativePath);
        }
    }

    TE_LOG_ERROR("[RHIOpenGL] Unknown logical shader '{}'", logicalName);
    return {};
}

} // namespace TE
