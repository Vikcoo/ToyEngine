# 架构优化复盘：Reversed-Z 深度约定

本文按增量方式记录 ToyEngine 将 GPU 主渲染路径切换到 Reversed-Z 的原因、边界和后续约束。CPU 相机与 Frustum 继续使用正向 `RH_ZO`，Renderer 在提交 GPU 前转换投影；两者不能混用。

## 第一阶段：主渲染路径切换（2026-07-14）

### 为什么要改

Deferred 的 `WorldPositionReconstructionError` 热力图显示：物体靠近相机时，深度重建位置与 `RGBA32_Float` Position RT 的差异多为微米级；相机远离后，误差逐步进入青、绿、黄区间，即从几十微米增长到毫米级。

旧路径使用普通 Z：近裁剪面映射到 0、远裁剪面映射到 1，深度清除为 1，测试使用 `Less`。透视投影本身已经把大量数值范围分配给近处，而 `D32_Float` 又在 0 附近拥有更密集的可表示值、在 1 附近更稀疏。两种分布叠加后，远处世界空间位置的重建精度下降明显。

Reversed-Z 原本的作用不是让深度线性化，也不是增加深度位数，而是把近裁剪面映射到 1、远裁剪面映射到 0，让浮点数在 0 附近更密集的分布用于远处深度。它通过抵消透视深度分布与浮点分布的同向偏斜，显著改善大范围场景的深度测试与深度重建精度。

### 核心设计与取舍

1. `CameraComponent` 继续生成普通 `PerspectiveRH_ZO()`，`FViewInfo::ViewProjectionMatrix` 与 `Frustum::FromViewProjectionRH_ZO()` 保持 Near=0、Far=1。CPU 剔除不依赖 GPU 精度策略。
2. Renderer 通过 `Matrix4::ReverseZProjectionZO()` 在 GPU 提交边界执行 `z' = w - z`，得到 Near=1、Far=0 的投影，再调用 `RHIDevice::AdjustProjectionMatrix()` 做后端 NDC 适配。
3. Forward BasePass 与 Deferred GBuffer 统一清除深度为 0，并使用 `Greater` 深度测试。
4. Deferred Lighting 将深度接近 0 视为无几何背景。深度重建继续使用调整后 Reversed-Z VP 的逆矩阵，因此不需要先把深度手工翻回普通 Z。
5. `WorldPosition` RT 暂时保留，正式光照仍读取存储位置；Reversed-Z 当前首先服务于深度精度校准和后续安全移除 Position RT 的准备工作。

选择在 Renderer 边界转换，而不是直接修改相机投影，是为了避免同时重写 Frustum 平面提取、CPU 可见性判断和既有数学测试。代价是以后任何新的 GPU 深度 Pass 都必须显式复用 `RendererDepthConvention`，不能直接提交 `FViewInfo::ProjectionMatrix`。

### 带来的收益

- `D32_Float` 的高精度区间用于远距离深度，降低远处表面深度量化和世界坐标重建误差。
- Forward 与 Deferred 共享同一套清除值、比较操作和 GPU 投影约定，避免路径漂移。
- CPU Frustum 契约不变，已有 `RH_ZO` 剔除逻辑与测试继续有效。
- 深度重建直接消费 Reversed-Z 深度和对应逆 VP，链路保持闭合，不引入额外线性化近似。

### 当前限制与后续检查

- 当前仍使用有限远裁剪面，尚未引入无限远 Reversed-Z 投影。
- `F7` 原始 Depth 视图现在是近处亮、远处暗；不能再按普通 Z 的颜色方向解释。
- Position RT 仍占用带宽，是否删除必须继续以 `F8` 误差视图和多场景测试为依据。
- 未来加入 Shadow Map、SSAO、SSR、深度金字塔、软粒子或深度 Resolve 时，都必须明确其深度方向、归约操作和背景值；例如 Reversed-Z Hi-Z 通常使用最大深度表达更近遮挡物。
- 若未来某后端无法使用 `[0,1]` 原生 NDC，仍须先完成 Reversed-Z ZO 投影，再由 `AdjustProjectionMatrix()` 映射到后端范围；重建时使用同一调整后矩阵。

### 验证

- `MathTest` 验证 `ReverseZProjectionZO()` 将 Near 映射到 1、Far 映射到 0。
- 原有 Frustum 测试继续使用普通 `PerspectiveRH_ZO()` 并通过。
- CLion MinGW Release 全量构建通过。

## 第二阶段：误差视图实测复核（2026-07-14）

### 观察结果

切换 Reversed-Z 后，`F8` 的存储世界坐标与重建世界坐标总误差颜色在远处没有明显下降。这个结果修正了第一阶段“总热力图应明显变暗”的过强预期，但不否定 Reversed-Z 已改善深度量化。

### 当前判断

用与 shader 相同的 float32 精度对当前 `Near=0.1 / Far=100` 做数值复算，普通 Z 在约 37–87 米处的逆 VP 重建误差可达到约 `0.6–7 mm`，Reversed-Z 对应结果约为 `1–3 μm`。因此投影与深度数值部分确实获得了预期收益。

当前 `F8` 比较的不是“纯深度量化误差”，而是两条完整 GPU 路径的总差异：

- Position RT：顶点世界坐标、透视校正 varying 插值、光栅化精度、`RGBA32_Float` 写入与采样。
- 重建位置：屏幕 UV、深度光栅化与 `D32_Float`、采样、float32 inverse VP、矩阵乘法和齐次除法。

Reversed-Z 只优化第二条路径中的深度表示部分。如果光栅插值、两种插值路径不完全一致、世界空间大数运算或逆 VP 数值误差成为主导项，总热力图就可能几乎不变。

### 后续诊断方向

下一阶段不应继续仅凭 `F8` 总误差判断 Reversed-Z 是否有效，而应拆分诊断量：

1. 增加“Position RT 重新投影深度 vs Depth RT”的深度专用误差视图，隔离深度写入与采样误差。
2. 上传 `InvProjection` 与 `InvView`，先重建观察空间位置，再转换到世界空间，与当前直接 `inverse(Projection * View)` 路径对比。
3. 将 XYZ 误差分量或观察射线方向/距离误差分开显示，判断误差来自深度方向还是屏幕方向。

在上述拆分完成前，Position RT 继续保留，不能依据 Reversed-Z 已启用就直接删除。

## 第三阶段：Deferred 正式光照切换到重建位置（2026-07-15）

### 目标与实现路径

在保留 Position RT 安全对照的前提下，让正式 Deferred Lighting 实际消费深度重建世界坐标，以直接观察视线方向、IBL 反射、点光方向和距离衰减的画面效果。Lighting shader 对有效几何像素使用 Reversed-Z Depth、后端 NDC 深度范围与 `inverse(adjusted Reversed-Z Projection * View)` 重建世界坐标；它不对深度做线性化，也不先翻回普通 Z。

`Lit` 分支不再采样 `u_GBufferWorldPosition`。Position RT 仍由 GBuffer Pass 写入并保持绑定，只在 `WorldPosition` 与 `WorldPositionReconstructionError` 调试视图中采样。这样既能让正式路径覆盖重建逻辑，又保留 F6/F8 对照和低成本回退入口。

### 取舍、收益与限制

- 收益：正式 Lighting 不再读取 16 字节/像素的 `RGBA32_Float` Position RT，视线、IBL 和点光计算统一验证重建位置；但 GBuffer Pass 仍写入该 RT，因此当前只减少 Lit Pass 的读取流量，尚未回收附件显存和写带宽。
- 取舍：没有立即删除 Position RT，也没有同时引入拆分的 `InvProjection + InvView` 路径，避免把效果验证与资源布局重构混在一起。
- 限制：F8 仍是两条完整路径的总误差，不能单独证明深度量化质量；世界空间大坐标、逆 VP 数值误差和屏幕边缘仍需实测。
- 优先检查：若出现点光随视角漂移、反射方向异常或物体边缘闪烁，先查看 F8，再检查 Reversed-Z 投影、深度清除/比较、背景阈值、NDC Z 转换与 RT Y 翻转是否仍成对一致。

后续只有在多距离、多屏幕区域和动态相机下确认正式光照稳定后，才进入删除 Position RT、绑定和 GBuffer attachment 的下一阶段。
