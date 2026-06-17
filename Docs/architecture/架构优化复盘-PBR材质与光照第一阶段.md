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

## 阶段二：材质 GPU 绑定与 Direct PBR（已完成，2026-06-17）

### 为什么继续改

第一阶段只把材质语义放进资产侧，渲染器仍然只绑定 BaseColor。继续推进 PBR 时，如果不先把材质参数、PBR 贴图和切线空间接入 GPU，BRDF shader 会缺少稳定输入，问题会混在材质、纹理、法线和光照公式之间。

### 核心改动

- `FStaticMeshVertex` 增加 Tangent，Assimp 导入时读取 Tangent，Sandbox fallback 立方体也补齐每个面的 Tangent。
- `FRenderResourceManager` 从 BaseColor 缓存升级为 `FPreparedMaterialTextures`，为 BaseColor / Normal / Metallic / Roughness / AO / Emissive 准备纹理。
- 增加路径级纹理缓存，缓存 key 包含规范化路径、颜色空间和贴图语义，避免多个网格重复上传同一贴图。
- 增加 `MaterialBlock`，上传 BaseColor / Metallic / Roughness / AO / Emissive 等 factor。
- Forward 与 Deferred GBuffer shader 消费 PBR 贴图和 Tangent-space normal。
- Deferred GBuffer 增加 Material RT，Lighting Pass 消费 Metallic / Roughness / AO / Emissive。

### 收益与限制

这一步让材质系统从“资产侧描述”进入“真实 GPU 绑定与 shader 消费”，Forward / Deferred 两条路径都能使用同一套 PBR 输入。限制是当时仍只有直接光，缺少环境照明，因此纯 PBR 画面在弱光或 AO 偏黑时明显偏暗；PBR 参数 DebugView 也尚未补齐。

## 阶段三：HDR 天空与运行时 IBL 初版（已完成，2026-06-18）

### 为什么继续改

Direct Lighting PBR 在没有 IBL、曝光和 Tonemapping 时会显著偏暗。单纯加常量环境光虽然能提亮物体，但不能验证“材质从环境 cubemap 获得光照贡献”的真实 PBR 路径。因此本阶段直接引入 HDR 环境图到 cubemap 的运行时预计算，并让 Forward / Deferred shader 采样 IBL 资源。

### 核心改动

- RHI 增加 `RHITextureDimension::TextureCube` 与 `RHIBindingType::TextureCube`。
- OpenGL 后端支持 `GL_TEXTURE_CUBE_MAP` 创建、绑定和 `samplerCube` 资源绑定。
- `RHISamplerDesc` 增加 `addressW`，cubemap 环境采样使用三轴 `ClampToEdge`。
- `FRenderResourceManager` 从 `Content/Textures/HDR/citrus_orchard_road_puresky_4k.hdr` 运行时加载 float HDR，并生成环境 cubemap、diffuse irradiance cubemap、BRDF LUT。
- Forward 新增全屏 Sky pass，用 inverse view-projection 采样环境 cubemap 绘制天空背景。
- Deferred Lighting Pass 对远平面像素采样天空 cubemap，并在 PBR 光照中采样 irradiance / prefilter / BRDF LUT。

### 设计取舍

本阶段接受运行时 CPU 预计算，先保证资源语义和 shader 消费链路完整。specular prefilter 当前仍复用环境 cubemap 的 mip 链，是能工作的初版近似，不等同于正式 GGX 重要性采样预滤波。这样做的好处是先把 RHI cubemap、Environment BindGroup、Forward/Deferred PBR 采样路径打通，后续可以只替换预计算质量，而不再重排主渲染路径。

### 剩余限制

- specular IBL 还不是正式 GGX prefiltered cubemap。
- IBL 预计算没有缓存落盘，每次运行首次准备资源时会重新计算。
- 天空背景只有局部 Reinhard 映射，项目仍缺正式曝光和 Tonemapping 后处理。
- IBL 资源路径当前是固定 HDR 文件，尚未接入资源注册表或场景级 SkyLight 配置。
