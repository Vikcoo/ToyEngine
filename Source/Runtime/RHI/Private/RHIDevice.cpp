// ToyEngine RHI Module
// RHIDevice 默认方法实现

#include "RHIDevice.h"

namespace TE {

Matrix4 RHIDevice::AdjustProjectionMatrix(const Matrix4& projection) const
{
    // 默认实现：引擎约定的 [0,1] 深度 + Y-up 投影矩阵无需修正。
    // Vulkan/D3D12 后端可以 override 此方法来做 Y 轴翻转等适配。
    return projection;
}

} // namespace TE
