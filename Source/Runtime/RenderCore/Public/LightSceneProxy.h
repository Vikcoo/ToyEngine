// ToyEngine RenderCore Module
// FLightSceneProxy - 光源组件的渲染侧镜像

#pragma once

#include "Math/MathTypes.h"

#include <cstdint>

namespace TE {

enum class ELightType : uint8_t
{
    Directional = 0,
    Point = 1,
};

struct FLightSceneProxy
{
    ELightType Type = ELightType::Directional;
    Vector3 Color = Vector3::One;
    float Intensity = 1.0f;
    Vector3 Direction = Vector3(0.5f, 1.0f, 0.8f).Normalize();
    Vector3 Position = Vector3::Zero;
    float AttenuationRadius = 10.0f;
};

} // namespace TE
