// ToyEngine Renderer Module
// RendererLightUniforms 实现

#include "RendererLightUniforms.h"

#include "LightSceneProxy.h"
#include "RendererScene.h"
#include "RHICommandBuffer.h"

#include <cstdint>
#include <cstdio>

namespace TE {

namespace {

constexpr uint32_t MaxDirectionalLights = 4;
constexpr uint32_t MaxPointLights = 8;

void SetIndexedVec3(RHICommandBuffer* cmdBuf, const char* baseName, uint32_t index, const Vector3& value)
{
    char uniformName[64];
    std::snprintf(uniformName, sizeof(uniformName), "%s[%u]", baseName, index);
    cmdBuf->SetUniformVec3(uniformName, &value.X);
}

void SetIndexedFloat(RHICommandBuffer* cmdBuf, const char* baseName, uint32_t index, float value)
{
    char uniformName[64];
    std::snprintf(uniformName, sizeof(uniformName), "%s[%u]", baseName, index);
    cmdBuf->SetUniformFloat(uniformName, value);
}

} // namespace

void BindSceneLightUniforms(const FScene* scene, RHICommandBuffer* cmdBuf)
{
    uint32_t directionalCount = 0;
    uint32_t pointCount = 0;

    if (scene)
    {
        for (const auto* light : scene->GetLights())
        {
            if (!light)
            {
                continue;
            }

            const Vector3 lightColor = light->Color * light->Intensity;
            if (light->Type == ELightType::Directional)
            {
                if (directionalCount >= MaxDirectionalLights)
                {
                    continue;
                }

                Vector3 direction = light->Direction.Normalize();
                if (direction.LengthSquared() <= 0.0f)
                {
                    direction = Vector3::Forward;
                }

                SetIndexedVec3(cmdBuf, "u_DirectionalLightDirections", directionalCount, direction);
                SetIndexedVec3(cmdBuf, "u_DirectionalLightColors", directionalCount, lightColor);
                ++directionalCount;
            }
            else if (light->Type == ELightType::Point)
            {
                if (pointCount >= MaxPointLights)
                {
                    continue;
                }

                SetIndexedVec3(cmdBuf, "u_PointLightPositions", pointCount, light->Position);
                SetIndexedVec3(cmdBuf, "u_PointLightColors", pointCount, lightColor);
                SetIndexedFloat(cmdBuf, "u_PointLightRadii", pointCount, light->AttenuationRadius);
                ++pointCount;
            }
        }
    }

    cmdBuf->SetUniformInt("u_DirectionalLightCount", static_cast<int32_t>(directionalCount));
    cmdBuf->SetUniformInt("u_PointLightCount", static_cast<int32_t>(pointCount));
}

} // namespace TE
