# PBR 渲染前置规划

本文档记录进入 PBR 渲染实现前建议先补齐的基础能力。除明确标注为“已完成”的条目外，其余内容仍是计划中，不代表当前代码已经实现。

## 背景

当前渲染器已经具备 Forward / Deferred 两条路径、静态网格 BaseColor 贴图链路、基础光源 UBO / BindGroup、GBuffer 调试视图和运行时渲染路径切换能力。

下一阶段如果直接把 Cook-Torrance BRDF 写进 shader，短期可以看到效果，但很容易因为材质数据、纹理颜色空间、切线空间和调试能力不足而反复返工。因此 PBR 应先作为材质、资源、渲染绑定和调试视图的一次小型体系化扩展推进。

## 阶段目标

下一阶段推荐目标是：

> 先完成最小 Forward PBR + 材质数据结构 + sRGB/Linear 纹理语义 + PBR 参数调试视图，再扩展 Deferred PBR 和 IBL。

这个目标的取舍是先把材质输入和直接光照跑通，避免同时修改材质系统、GBuffer 布局、Lighting Pass、IBL 预计算和资产导入规则。

## 必须先补齐的能力

### 1. 最小材质系统（已完成第一步）

资产侧最小材质描述已经落地：`StaticMesh` 当前持有 `FMaterial` 列表，用于表达同一个网格不同 Section 的材质输入。当前 Renderer 仍只消费 BaseColor 贴图，其它字段先作为后续 PBR 绑定与 shader 接入的基础。

建议最小字段：
- `BaseColorFactor`
- `MetallicFactor`
- `RoughnessFactor`
- `EmissiveFactor`
- `BaseColorTexture`
- `MetallicRoughnessTexture` 或独立 `MetallicTexture` / `RoughnessTexture`
- `NormalTexture`
- `AOTexture`
- `EmissiveTexture`

已落地部分：
- `FMaterial` 保存 BaseColor / Metallic / Roughness / AO / Emissive 标量因子。
- `FMaterial` 保存 BaseColor / MetallicRoughness / Metallic / Roughness / Normal / AO / Emissive 贴图槽。
- 贴图槽保存用途和 sRGB / Linear 颜色空间语义。
- `FAssetImporter` 从 Assimp 材质转换为引擎自有 `FMaterial`，Renderer 不直接依赖 Assimp 材质结构。

剩余工作：
- Renderer 仍未创建 `MaterialBlock`。
- Renderer 仍未绑定 BaseColor 之外的 PBR 贴图。
- shader 仍未消费 Metallic / Roughness / Normal / AO / Emissive。
- 除 BaseColor 白纹理外，其它 PBR 贴图默认资源尚未建立。

### 2. 纹理语义与颜色空间

PBR 对颜色空间非常敏感，必须在资源描述中显式区分纹理用途。

建议规则：
- `BaseColor` / `Emissive` 使用 sRGB 采样或加载时转为线性空间。
- `Normal` / `Metallic` / `Roughness` / `AO` 使用 Linear 采样。
- 渲染计算统一在线性空间中进行。
- 最终输出到默认帧缓冲前，需明确由 shader 或后端状态负责线性到 sRGB 的转换。

如果这一层没有先固定，PBR 结果会出现“整体偏灰、偏暗、粗糙度不稳定、金属反射不对”等难以定位的问题。

### 3. 切线空间

Normal Map 需要稳定的 Tangent 基础。PBR 前应让静态网格顶点数据支持 Tangent，优先从 Assimp 导入，缺失时再考虑生成。

建议要求：
- `StaticMesh` 顶点结构新增或预留 Tangent。
- Assimp 导入时读取 Tangent 数据。
- 缺失 Tangent 的资产先允许退化为几何法线渲染，但调试日志应能提示。
- Normal Map 解码统一在切线空间到世界空间的路径中处理。

### 4. PBR 参数调试视图

PBR 调试不能只看最终 Lit 结果。下一阶段应在已有 DebugView 基础上扩展参数可视化。

建议调试视图：
- BaseColor
- Normal / WorldNormal
- Metallic
- Roughness
- AO
- Emissive
- DirectDiffuse
- DirectSpecular

Forward PBR 阶段可以先在 Forward shader 中支持关键视图；Deferred PBR 阶段再把它们映射到 GBuffer / Lighting Pass。

## 建议的绑定模型

当前项目已经引入 `BindGroupLayout` / `PipelineLayout`，PBR 阶段应继续收敛资源绑定形状，而不是在不同 shader 中散落绑定约定。

建议拆分：
- `ObjectBlock`：每个 draw 的 `Model`、`MVP`、`NormalMatrix`
- `ViewBlock`：相机位置、View、Projection、ViewProjection
- `LightBlock`：方向光、点光及数量
- `MaterialBlock`：BaseColor、Metallic、Roughness、AO、Emissive 等标量参数
- `MaterialTextureGroup`：BaseColor、Normal、MetallicRoughness、AO、Emissive 等纹理与采样器

材质资源绑定应尽量由 `FRenderResourceManager` 或后续独立材质资源子系统统一创建，避免 Draw 提交阶段临时拼装过多资源布局。

## Forward 与 Deferred 的实施顺序

推荐顺序：

1. 在 Forward 路径完成 Direct Lighting PBR。
2. 验证 BaseColor / Metallic / Roughness / Normal / AO 输入。
3. 增加 PBR 参数调试视图。
4. 扩展 Deferred GBuffer 布局，再迁移 PBR Lighting Pass。
5. 最后补 IBL。

推荐先做 Forward 的原因是变量更少：Forward 路径可以直接从材质输入到 BRDF 输出，便于验证材质、纹理语义、法线空间和光照公式。Deferred PBR 会同时牵涉 GBuffer 编码、Lighting Pass 解码、调试视图和 RT 格式选择，适合在材质基础稳定后推进。

## Deferred PBR 的计划中 GBuffer

计划中的 GBuffer 布局可以先按最小需求设计：
- `GBuffer0`：BaseColor.rgb + AO
- `GBuffer1`：WorldNormal 或压缩 Normal
- `GBuffer2`：Metallic + Roughness + Specular/Flags
- `Depth`：深度

当前 `WorldPosition` RT 可在早期保留以降低实现难度；后续可改为通过 Depth + View 参数重建世界位置，减少带宽。

## 光照约定

PBR 不要求下一阶段立即采用完整物理单位，但必须固定项目内约定。

建议：
- BRDF 使用 GGX / Smith / Schlick Fresnel 的常见 Cook-Torrance 模型。
- DirectionalLight 的强度先作为无单位倍率处理。
- PointLight 先使用可控的 inverse-square 或带半径裁剪的衰减公式，文档中明确公式。
- 先保证材质响应稳定，再考虑真实单位、曝光和 Tonemapping。

## IBL 阶段规划

纯直接光 PBR 画面偏黑是正常现象，不应误判为 BRDF 错误。IBL 建议作为后续阶段推进：

1. Diffuse irradiance。
2. Prefiltered specular cubemap。
3. BRDF LUT。
4. 天空盒 / 环境贴图资产导入。
5. 曝光与 Tonemapping。

## 资产导入建议

后续资源管线应优先对齐 glTF 2.0 PBR 语义，而不是自定义贴图命名规则。

优先读取：
- BaseColor
- MetallicRoughness
- Normal
- Occlusion
- Emissive

Assimp 能提供的材质键应统一转换到引擎自己的材质描述中，Renderer 不应直接依赖 Assimp 材质结构。

## 验收标准

进入 Deferred PBR 或 IBL 前，Forward PBR 阶段至少应满足：
- 无贴图材质可以通过标量参数稳定渲染。
- BaseColor 使用 sRGB/Linear 规则后，颜色与预期一致。
- Metallic / Roughness 参数变化可观察且符合直觉。
- Normal Map 在带非等比缩放的模型上仍方向正确。
- DebugView 能单独查看 BaseColor、Normal、Metallic、Roughness。
- 同一模型多个 Section 可使用不同材质输入。

## 后续文档要求

正式实现 PBR 时，需要同步更新：
- `Docs/guides/渲染管线.md`：只记录已经落地的 PBR 路径与调试视图。
- `Docs/architecture/运行时模块.md`：更新 Asset / RenderCore / Renderer 的材质职责。
- `Docs/guides/资源导入流程.md`：记录 glTF / Assimp PBR 材质导入规则。
- `Docs/reference/临时架构问题清单.md`：记录阶段后仍未解决的结构性限制。

若实现过程中遇到颜色空间、Normal Map、GBuffer 编码或纹理采样相关 bug，还必须同步更新 `Docs/reference/图形与开发踩坑记录.md`。
