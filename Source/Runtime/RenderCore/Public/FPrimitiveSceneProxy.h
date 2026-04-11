// ToyEngine RenderCore Module
// FPrimitiveSceneProxy - 可渲染组件的渲染侧镜像
// 对应 UE5 的 FPrimitiveSceneProxy

#pragma once

#include "FMeshDrawCommand.h"
#include "Math/MathTypes.h"

#include <vector>

namespace TE {

class FPrimitiveSceneProxy
{
public:
    virtual ~FPrimitiveSceneProxy() = default;

    void SetWorldMatrix(const Matrix4& matrix) { m_WorldMatrix = matrix; }
    [[nodiscard]] const Matrix4& GetWorldMatrix() const { return m_WorldMatrix; }

    virtual void GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const = 0;

protected:
    FPrimitiveSceneProxy() = default;

    Matrix4 m_WorldMatrix = Matrix4::Identity;
};

} // namespace TE
