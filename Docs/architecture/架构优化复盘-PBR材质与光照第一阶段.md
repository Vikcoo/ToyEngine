# 架构优化复盘：PBR 材质与光照（第一阶段）

## 背景与问题

PBR 渲染不能只依赖 shader 公式。进入 BRDF、Normal Map、IBL 之前，资源侧必须先能稳定表达材质输入。

本阶段前，静态网格材质槽只保存 BaseColor 贴图路径。这会导致：
- Metallic / Roughness / Normal / AO / Emissive 没有资产侧承载位置。
- BaseColor 这类 sRGB 数据与 Normal / Roughness 这类线性数据没有统一语义。
- Renderer 后续如果直接读取 Assimp 材质结构，会破坏 `Asset` 模块封装边界。

## 方案与取舍

第一阶段只实现资产侧最小材质系统，不改 shader 和渲染结果。

核心改动：
- 新增 `FMaterial`，保存 BaseColor / Metallic / Roughness / AO / Emissive 标量因子。
- 新增 `FMaterialTextureSlot`，保存贴图路径、贴图用途和 sRGB / Linear 颜色空间语义。
- `StaticMesh` 的材质槽从仅 BaseColor 路径升级为 `FMaterial` 列表。
- `FAssetImporter` 把 Assimp 材质转换为引擎自有 `FMaterial`。
- `FRenderResourceManager` 当前继续只消费 `FMaterial` 的 BaseColor 贴图，保持现有 Forward / Deferred 输出不变。

本阶段没有引入 `MaterialBlock` UBO，也没有绑定 Metallic / Roughness / Normal 等 PBR 贴图。这样做是为了把资源语义先落地，避免和 PBR shader、GBuffer 布局、DebugView 同时变化。

## 收益

- 材质数据从“BaseColor 路径字段”升级为可扩展的最小 PBR 描述。
- Assimp 仍被限制在 `Asset` 私有实现中，Renderer 不需要知道 Assimp 材质结构。
- 颜色空间语义开始进入资产描述，为后续 sRGB / Linear 纹理创建策略提供依据。
- 现有 BaseColor 渲染链路保持兼容，降低第一阶段风险。

## 限制与后续方向

- Renderer 仍只准备和绑定 BaseColor 纹理。
- `MaterialBlock`、PBR 纹理 BindGroup、BRDF shader 尚未实现。
- StaticMesh 顶点数据尚未包含 Tangent，Normal Map 仍不能正确接入。
- 除默认白色 BaseColor 纹理外，其它 PBR 默认贴图资源尚未建立。

下一阶段建议：
1. 按 `FMaterialTextureSlot` 的颜色空间语义创建 GPU 纹理。
2. 增加材质参数 `MaterialBlock`。
3. 补齐 StaticMesh Tangent 导入。
4. 先在 Forward 路径实现 Direct Lighting PBR 和 PBR 参数 DebugView。
