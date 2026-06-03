// ToyEngine Renderer Module
// RendererLightUniforms 实现

#include "RendererLightUniforms.h"

#include "LightSceneProxy.h"
#include "RendererScene.h"
#include "RHIBindGroup.h"
#include "RHIBuffer.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"

#include <array>
#include <cstdint>

namespace TE {

namespace {

constexpr uint32_t MaxDirectionalLights = 4;
constexpr uint32_t MaxPointLights = 8;
constexpr uint32_t LightBlockBinding = 0;

struct alignas(16) FLightBlockCPU
{
    std::array<int32_t, 4> Counts = {0, 0, 0, 0};
    std::array<Vector4, MaxDirectionalLights> DirectionalLightDirections = {};
    std::array<Vector4, MaxDirectionalLights> DirectionalLightColors = {};
    std::array<Vector4, MaxPointLights> PointLightPositions = {};
    std::array<Vector4, MaxPointLights> PointLightColorsAndRadii = {};
};

static_assert(sizeof(FLightBlockCPU) % 16 == 0);

void FillLightBlockFromScene(const FScene* scene, FLightBlockCPU& outBlock)
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

                outBlock.DirectionalLightDirections[directionalCount] = Vector4(direction.X, direction.Y, direction.Z, 0.0f);
                outBlock.DirectionalLightColors[directionalCount] = Vector4(lightColor.X, lightColor.Y, lightColor.Z, 0.0f);
                ++directionalCount;
            }
            else if (light->Type == ELightType::Point)
            {
                if (pointCount >= MaxPointLights)
                {
                    continue;
                }

                outBlock.PointLightPositions[pointCount] = Vector4(light->Position.X, light->Position.Y, light->Position.Z, 1.0f);
                outBlock.PointLightColorsAndRadii[pointCount] = Vector4(lightColor.X, lightColor.Y, lightColor.Z, light->AttenuationRadius);
                ++pointCount;
            }
        }
    }

    outBlock.Counts[0] = static_cast<int32_t>(directionalCount);
    outBlock.Counts[1] = static_cast<int32_t>(pointCount);
}

} // namespace

bool EnsureLightUniformBindingState(RHIDevice* device, FLightUniformBindingState& state)
{
    if (!device)
    {
        return false;
    }

    if (state.UniformBuffer && state.BindGroup)
    {
        return true;
    }

    RHIBufferDesc bufferDesc;
    bufferDesc.usage = RHIBufferUsage::Uniform;
    bufferDesc.size = sizeof(FLightBlockCPU);
    bufferDesc.debugName = "Renderer_LightBlock_UBO";
    state.UniformBuffer = device->CreateBuffer(bufferDesc);
    if (!state.UniformBuffer)
    {
        return false;
    }

    RHIBindGroupDesc bindGroupDesc;
    bindGroupDesc.debugName = "Renderer_LightBlock_BindGroup";
    bindGroupDesc.entries.push_back({
        LightBlockBinding,
        RHIBindingType::UniformBuffer,
        state.UniformBuffer.get(),
        0,
        sizeof(FLightBlockCPU),
        nullptr,
        nullptr
    });

    state.BindGroup = device->CreateBindGroup(bindGroupDesc);
    return state.BindGroup && state.BindGroup->IsValid();
}

bool UpdateAndBindSceneLightUniforms(const FScene* scene,
                                     RHIDevice* device,
                                     RHICommandBuffer* cmdBuf,
                                     FLightUniformBindingState& state)
{
    if (!cmdBuf || !EnsureLightUniformBindingState(device, state) || !state.UniformBuffer || !state.BindGroup)
    {
        return false;
    }

    FLightBlockCPU lightBlock;
    FillLightBlockFromScene(scene, lightBlock);

    if (!state.UniformBuffer->UpdateData(&lightBlock, sizeof(lightBlock), 0))
    {
        return false;
    }

    cmdBuf->SetBindGroup(0, state.BindGroup.get());
    return true;
}

} // namespace TE
