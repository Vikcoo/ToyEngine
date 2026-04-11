// ToyEngine Scene Module
// 游戏侧到渲染侧的场景接口

#pragma once

#include "Math/MathTypes.h"

#include <cstdint>
#include <memory>

namespace TE {

class TStaticMesh;

using RenderPrimitiveHandle = std::uint64_t;
constexpr RenderPrimitiveHandle InvalidRenderPrimitiveHandle = 0;

enum class RenderPrimitiveKind
{
    StaticMesh
};

struct RenderPrimitiveCreateInfo
{
    RenderPrimitiveKind Kind = RenderPrimitiveKind::StaticMesh;
    std::shared_ptr<TStaticMesh> StaticMesh;
    Matrix4 WorldMatrix = Matrix4::Identity;
};

/// 游戏线程视角的渲染场景接口。
/// Scene 模块只依赖该接口，不直接依赖 Renderer 的具体实现类型。
class IRenderScene
{
public:
    virtual ~IRenderScene() = default;

    [[nodiscard]] virtual RenderPrimitiveHandle CreatePrimitive(const RenderPrimitiveCreateInfo& createInfo) = 0;
    virtual void UpdatePrimitiveTransform(RenderPrimitiveHandle handle, const Matrix4& worldMatrix) = 0;
    virtual void DestroyPrimitive(RenderPrimitiveHandle handle) = 0;
};

} // namespace TE
