# PBR 渲染前置规划

本文档记录进入 PBR 渲染实现前建议先补齐的基础能力。除明确标注为“已完成”的条目外，其余内容仍是计划中，不代表当前代码已经实现。

## 背景

当前渲染器已经具备 Forward / Deferred 两条路径、静态网格 PBR 材质贴图绑定、基础光源 UBO / BindGroup、GBuffer 调试视图和运行时渲染路径切换能力。

下一阶段如果直接把 Cook-Torrance BRDF 写进 shader，短期可以看到效果，但很容易因为材质数据、纹理颜色空间、切线空间和调试能力不足而反复返工。因此 PBR 应先作为材质、资源、渲染绑定和调试视图的一次小型体系化扩展推进。

## 阶段目标

下一阶段推荐目标是：

> 先完成最小 Forward PBR + 材质数据结构 + sRGB/Linear 纹理语义 + PBR 参数调试视图，再扩展 Deferred PBR 和 IBL。

这个目标的取舍是先把材质输入和直接光照跑通，避免同时修改材质系统、GBuffer 布局、Lighting Pass、IBL 预计算和资产导入规则。

## 必须先补齐的能力

### 1. 最小材质系统（已完成第二步）

资产侧最小材质描述已经落地：`StaticMesh` 当前持有 `FMaterial` 列表，用于表达同一个网格不同 Section 的材质输入。Renderer 当前已经创建 `MaterialBlock`，并绑定 BaseColor / Normal / Metallic / Roughness / AO / Emissive 六类 PBR 贴图供 Forward 与 Deferred 路径消费。

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
- `FRenderResourceManager` 按材质槽准备 `FPreparedMaterialTextures`，并按规范化纹理路径、颜色空间和语义复用 GPU 纹理。
- Forward / Deferred GBuffer Pass 通过 `MaterialTextureGroup` 绑定 BaseColor / Normal / Metallic / Roughness / AO / Emissive。
- Forward BasePass 与 Deferred Lighting Pass 已使用 GGX / Smith / Schlick Fresnel 的直接光 PBR。
- 缺失贴图时使用默认白、黑和法线贴图，避免 shader 分支依赖空资源。

剩余工作：
- `MetallicRoughnessTexture` 打包贴图尚未在 shader 中解包消费，当前优先支持独立 Metallic / Roughness 贴图。
- Height / Parallax / Displacement 尚未接入，当前 `Content/Textures/PBR/RedRock04_4K_Height.png` 只作为后续资源保留。
- PBR 参数专用 DebugView 尚未补齐，目前 Deferred 仍只提供 Lit / Albedo / Normal / WorldPosition / Depth。
- 运行时 IBL 初版已实现：从单张 HDR equirectangular 图生成环境 cubemap、diffuse irradiance cubemap 和 BRDF LUT，并在 Forward / Deferred PBR 中采样。
- 当前 specular prefilter 仍是环境 cubemap mip 链近似，尚未实现 GGX 重要性采样预滤波 cubemap。
- 曝光和 Tonemapping 尚未作为正式后处理实现，天空 shader 当前只做局部 Reinhard 映射。

### 2. 纹理语义与颜色空间

PBR 对颜色空间非常敏感，必须在资源描述中显式区分纹理用途。

建议规则：
- `BaseColor` / `Emissive` 使用 sRGB 采样或加载时转为线性空间。
- `Normal` / `Metallic` / `Roughness` / `AO` 使用 Linear 采样。
- 渲染计算统一在线性空间中进行。
- 最终输出到默认帧缓冲前，需明确由 shader 或后端状态负责线性到 sRGB 的转换。

如果这一层没有先固定，PBR 结果会出现“整体偏灰、偏暗、粗糙度不稳定、金属反射不对”等难以定位的问题。

相关基础概念见：`Docs/knowledge/PBR材质与颜色空间基础.md`。

### 3. 切线空间（已完成基础接入）

Normal Map 需要稳定的 Tangent 基础。PBR 前应让静态网格顶点数据支持 Tangent，优先从 Assimp 导入，缺失时再考虑生成。

建议要求：
- `StaticMesh` 顶点结构已新增 Tangent。
- Assimp 导入时读取 Tangent 数据，缺失时使用默认 `(1, 0, 0)`。
- Sandbox 内置 fallback 立方体已补齐每个面的 Tangent。
- Forward / Deferred GBuffer shader 已使用 Tangent、Normal 和 UV 构造 TBN，将 Normal Map 从切线空间转换到世界空间。
- 后续仍应补充缺失 Tangent 的日志提示或离线生成策略。

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
- `EnvironmentGroup`：Irradiance cubemap、Prefilter cubemap、BRDF LUT

材质资源绑定应尽量由 `FRenderResourceManager` 或后续独立材质资源子系统统一创建，避免 Draw 提交阶段临时拼装过多资源布局。

## Forward 与 Deferred 的实施顺序

推荐顺序：

1. ~~在 Forward 路径完成 Direct Lighting PBR。~~（已完成）
2. ~~验证 BaseColor / Metallic / Roughness / Normal / AO 输入。~~（已完成基础接入）
3. 增加 PBR 参数调试视图。
4. ~~扩展 Deferred GBuffer 布局，再迁移 PBR Lighting Pass。~~（已完成最小直接光版本）
5. ~~最后补 IBL。~~（已完成 CPU 运行时初版，仍需升级 specular GGX 预滤波）

推荐先做 Forward 的原因是变量更少：Forward 路径可以直接从材质输入到 BRDF 输出，便于验证材质、纹理语义、法线空间和光照公式。Deferred PBR 会同时牵涉 GBuffer 编码、Lighting Pass 解码、调试视图和 RT 格式选择，适合在材质基础稳定后推进。

## Deferred PBR 的当前 GBuffer

当前 Deferred PBR 使用以下最小布局：
- `GBuffer0`：BaseColor.rgb + AO。
- `GBuffer1`：WorldNormal 编码到 0..1。
- `GBuffer2`：WorldPosition.xyz。
- `GBuffer3`：Metallic、Roughness、AO、Emissive Luma。
- `Depth`：深度。

当前 `WorldPosition` RT 继续保留，以避免深度重建误差影响点光方向与衰减。后续可在投影/深度约定稳定后再评估通过 Depth + View 参数重建世界位置，减少带宽。

## 光照约定

PBR 不要求下一阶段立即采用完整物理单位，但必须固定项目内约定。

建议：
- BRDF 使用 GGX / Smith / Schlick Fresnel 的常见 Cook-Torrance 模型。
- DirectionalLight 的强度先作为无单位倍率处理。
- PointLight 先使用可控的 inverse-square 或带半径裁剪的衰减公式，文档中明确公式。
- 先保证材质响应稳定，再考虑真实单位、曝光和 Tonemapping。

## IBL 阶段规划

纯直接光 PBR 画面偏黑是正常现象，不应误判为 BRDF 错误。当前已经实现 IBL 初版：

- 输入资源：`Content/Textures/HDR/citrus_orchard_road_puresky_4k.hdr`。
- 预计算方式：运行时 CPU 预计算。
- 输出资源：环境 cubemap、diffuse irradiance cubemap、初版 prefilter cubemap、BRDF LUT。
- Forward：先绘制 Sky pass，再绘制静态网格 PBR。
- Deferred：Lighting Pass 采样 IBL；深度为远平面的像素采样天空 cubemap 作为背景。

后续 IBL 质量升级建议：

1. 将 specular prefilter 从“环境 cubemap mip 近似”升级为 GGX 重要性采样预滤波。
2. 将 CPU 预计算替换或补充为 GPU 离屏预计算 Pass。
3. 增加环境资源选择、缓存落盘和热重载。
4. 增加曝光与 Tonemapping 后处理。
5. 增加 IBL DebugView。

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

继续提升 IBL 与 PBR 质量前，当前阶段至少应继续补齐：
- 无贴图材质可以通过标量参数稳定渲染。
- BaseColor 使用 sRGB/Linear 规则后，颜色与预期一致。
- Metallic / Roughness 参数变化可观察且符合直觉。
- Normal Map 在带非等比缩放的模型上仍方向正确。
- DebugView 能单独查看 BaseColor、Normal、Metallic、Roughness、AO、Emissive。
- 同一模型多个 Section 可使用不同材质输入。

## 后续文档要求

正式实现 PBR 时，需要同步更新：
- `Docs/guides/渲染管线.md`：只记录已经落地的 PBR 路径与调试视图。
- `Docs/architecture/运行时模块.md`：更新 Asset / RenderCore / Renderer 的材质职责。
- `Docs/guides/资源导入流程.md`：记录 glTF / Assimp PBR 材质导入规则。
- `Docs/reference/临时架构问题清单.md`：记录阶段后仍未解决的结构性限制。

若实现过程中遇到颜色空间、Normal Map、GBuffer 编码或纹理采样相关 bug，还必须同步更新 `Docs/reference/图形与开发踩坑记录.md`。
