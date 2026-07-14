# Shader 资源绑定清单

本文是 `Shader 反射与资源绑定描述推进规划` 的阶段一产物，用于记录当前 shader 侧声明与 C++ 侧资源绑定描述的基准状态。它不是自动反射结果，而是对当前手写约定的人工盘点，后续阶段的校验工具应以本文中的结构为初始对照。

当前仓库仍以 OpenGL GLSL 作为实际运行 shader，Renderer 通过逻辑 shader 名称请求 shader，`RHIOpenGL` 再映射到 `Content/Shaders/OpenGL/` 下的文件。

## 逻辑 Shader 映射

| 逻辑名称 | Stage | OpenGL GLSL 文件 | 当前用途 |
| --- | --- | --- | --- |
| `StaticMesh/BasePassVS` | Vertex | `model.vert` | Forward 静态网格 BasePass |
| `StaticMesh/BasePassPS` | Fragment | `model.frag` | Forward 静态网格 BasePass |
| `StaticMesh/GBufferVS` | Vertex | `gbuffer.vert` | Deferred GBuffer Pass |
| `StaticMesh/GBufferPS` | Fragment | `gbuffer.frag` | Deferred GBuffer Pass |
| `Deferred/LightingVS` | Vertex | `deferred_lighting.vert` | Deferred Lighting 全屏三角形 |
| `Deferred/LightingPS` | Fragment | `deferred_lighting.frag` | Deferred Lighting Pass |
| `Sky/FullscreenVS` | Vertex | `deferred_lighting.vert` | Sky 全屏三角形，复用 Deferred 顶点 shader |
| `Sky/SkyPS` | Fragment | `sky.frag` | Forward Sky Pass |

对应 C++ 位置：

- `Source/Runtime/RHIOpenGL/Private/OpenGLShaderLibrary.cpp`
- `Source/Runtime/Renderer/Private/RendererShaderNames.h`

## Renderer BindGroup 约定

当前 C++ 侧 group index 定义在 `RendererBindGroups` 中。

| Group 名称 | Group index | 当前资源类型 | 当前用途 |
| --- | ---: | --- | --- |
| `LightBlock` | 0 | UniformBuffer | Forward BasePass / Deferred Lighting 的光源数据 |
| `PassBlock` | 1 | UniformBuffer | ObjectBlock、DeferredPassBlock、SkyBlock，按 pipeline 语境复用 |
| `MaterialTextures` | 2 | Texture2D 组 | Forward BasePass / Deferred GBuffer 的材质贴图 |
| `MaterialBlock` | 3 | UniformBuffer | Forward BasePass / Deferred GBuffer 的材质参数 |
| `Environment` | 4 | TextureCube / Texture2D 组 | IBL 环境资源与天空资源 |
| `GBufferTextures` | 2 | Texture2D 组 | Deferred Lighting 的 GBuffer 输入 |

注意：`MaterialTextures` 与 `GBufferTextures` 当前都使用 group index `2`，因为它们不会在同一个 PipelineLayout 中同时出现。后续做自动生成或跨后端校验时，不能只按 group index 判断全局唯一性，必须结合 pipeline 语境。

## Renderer Binding Slot 约定

当前 C++ 侧 binding slot 定义在 `RendererBindings` 中。

| Binding 名称 | Binding slot | RHI 类型 | Shader 侧名称 |
| --- | ---: | --- | --- |
| `LightBlock` | 0 | `UniformBuffer` | `LightBlock` |
| `PassBlock` | 1 | `UniformBuffer` | `ObjectBlock` / `DeferredPassBlock` / `SkyBlock` |
| `BaseColorTexture` | 2 | `Texture2D` | `u_BaseColorTex` |
| `NormalTexture` | 3 | `Texture2D` | `u_NormalTex` |
| `MetallicTexture` | 4 | `Texture2D` | `u_MetallicTex` |
| `RoughnessTexture` | 5 | `Texture2D` | `u_RoughnessTex` |
| `AOTexture` | 6 | `Texture2D` | `u_AOTex` |
| `EmissiveTexture` | 7 | `Texture2D` | `u_EmissiveTex` |
| `MaterialBlock` | 8 | `UniformBuffer` | `MaterialBlock` |
| `IrradianceMap` | 9 | `TextureCube` | `u_IrradianceMap` |
| `PrefilterMap` | 10 | `TextureCube` | `u_PrefilterMap` |
| `BRDFLUT` | 11 | `Texture2D` | `u_BRDFLUT` |
| `GBufferAlbedo` | 2 | `Texture2D` | `u_GBufferAlbedo` |
| `GBufferNormal` | 3 | `Texture2D` | `u_GBufferNormal` |
| `GBufferWorldPosition` | 4 | `Texture2D` | `u_GBufferWorldPosition` |
| `GBufferDepth` | 5 | `Texture2D` | `u_GBufferDepth` |
| `GBufferMaterial` | 6 | `Texture2D` | `u_GBufferMaterial` |

注意：材质贴图 binding `2..7` 与 GBuffer 输入 binding `2..6` 共享编号区间，依赖不同 PipelineLayout 语境隔离。后续如果引入 Vulkan descriptor set / binding，应明确表达 set / group，而不是继续只依赖 OpenGL 风格的 binding。

## Shader 侧资源声明

### `model.vert`

| 类型 | 名称 | Location / Binding | Stage | C++ 对应 |
| --- | --- | ---: | --- | --- |
| VertexInput | `aPosition` | location 0 | Vertex | `FStaticMeshVertex::Position` |
| VertexInput | `aNormal` | location 1 | Vertex | `FStaticMeshVertex::Normal` |
| VertexInput | `aTexCoord` | location 2 | Vertex | `FStaticMeshVertex::TexCoord` |
| VertexInput | `aColor` | location 3 | Vertex | `FStaticMeshVertex::Color` |
| VertexInput | `aTangent` | location 4 | Vertex | `FStaticMeshVertex::Tangent` |
| UniformBuffer | `ObjectBlock` | binding 1 | Vertex | `RendererBindings::PassBlock` |

### `model.frag`

| 类型 | 名称 | Binding | Stage | C++ 对应 |
| --- | --- | ---: | --- | --- |
| UniformBuffer | `LightBlock` | 0 | Fragment | `RendererBindings::LightBlock` |
| UniformBuffer | `MaterialBlock` | 8 | Fragment | `RendererBindings::MaterialBlock` |
| Texture2D | `u_BaseColorTex` | 2 | Fragment | `RendererBindings::BaseColorTexture` |
| Texture2D | `u_NormalTex` | 3 | Fragment | `RendererBindings::NormalTexture` |
| Texture2D | `u_MetallicTex` | 4 | Fragment | `RendererBindings::MetallicTexture` |
| Texture2D | `u_RoughnessTex` | 5 | Fragment | `RendererBindings::RoughnessTexture` |
| Texture2D | `u_AOTex` | 6 | Fragment | `RendererBindings::AOTexture` |
| Texture2D | `u_EmissiveTex` | 7 | Fragment | `RendererBindings::EmissiveTexture` |
| TextureCube | `u_IrradianceMap` | 9 | Fragment | `RendererBindings::IrradianceMap` |
| TextureCube | `u_PrefilterMap` | 10 | Fragment | `RendererBindings::PrefilterMap` |
| Texture2D | `u_BRDFLUT` | 11 | Fragment | `RendererBindings::BRDFLUT` |

### `gbuffer.vert`

| 类型 | 名称 | Location / Binding | Stage | C++ 对应 |
| --- | --- | ---: | --- | --- |
| VertexInput | `aPosition` | location 0 | Vertex | `FStaticMeshVertex::Position` |
| VertexInput | `aNormal` | location 1 | Vertex | `FStaticMeshVertex::Normal` |
| VertexInput | `aTexCoord` | location 2 | Vertex | `FStaticMeshVertex::TexCoord` |
| VertexInput | `aColor` | location 3 | Vertex | `FStaticMeshVertex::Color` |
| VertexInput | `aTangent` | location 4 | Vertex | `FStaticMeshVertex::Tangent` |
| UniformBuffer | `ObjectBlock` | binding 1 | Vertex | `RendererBindings::PassBlock` |

### `gbuffer.frag`

| 类型 | 名称 | Binding / Location | Stage | C++ 对应 |
| --- | --- | ---: | --- | --- |
| UniformBuffer | `MaterialBlock` | binding 8 | Fragment | `RendererBindings::MaterialBlock` |
| Texture2D | `u_BaseColorTex` | binding 2 | Fragment | `RendererBindings::BaseColorTexture` |
| Texture2D | `u_NormalTex` | binding 3 | Fragment | `RendererBindings::NormalTexture` |
| Texture2D | `u_MetallicTex` | binding 4 | Fragment | `RendererBindings::MetallicTexture` |
| Texture2D | `u_RoughnessTex` | binding 5 | Fragment | `RendererBindings::RoughnessTexture` |
| Texture2D | `u_AOTex` | binding 6 | Fragment | `RendererBindings::AOTexture` |
| Texture2D | `u_EmissiveTex` | binding 7 | Fragment | `RendererBindings::EmissiveTexture` |
| FragmentOutput | `outAlbedo` | location 0 | Fragment | GBuffer color attachment 0 |
| FragmentOutput | `outNormal` | location 1 | Fragment | GBuffer color attachment 1 |
| FragmentOutput | `outWorldPosition` | location 2 | Fragment | GBuffer color attachment 2 |
| FragmentOutput | `outMaterial` | location 3 | Fragment | GBuffer color attachment 3 |

### `deferred_lighting.vert`

当前没有显式 vertex input、uniform block、sampler 或 fragment output 声明。它使用 `gl_VertexID` 生成全屏三角形。

### `deferred_lighting.frag`

| 类型 | 名称 | Binding | Stage | C++ 对应 |
| --- | --- | ---: | --- | --- |
| Texture2D | `u_GBufferAlbedo` | 2 | Fragment | `RendererBindings::GBufferAlbedo` |
| Texture2D | `u_GBufferNormal` | 3 | Fragment | `RendererBindings::GBufferNormal` |
| Texture2D | `u_GBufferWorldPosition` | 4 | Fragment | `RendererBindings::GBufferWorldPosition` |
| Texture2D | `u_GBufferDepth` | 5 | Fragment | `RendererBindings::GBufferDepth` |
| Texture2D | `u_GBufferMaterial` | 6 | Fragment | `RendererBindings::GBufferMaterial` |
| TextureCube | `u_IrradianceMap` | 9 | Fragment | `RendererBindings::IrradianceMap` |
| TextureCube | `u_PrefilterMap` | 10 | Fragment | `RendererBindings::PrefilterMap` |
| Texture2D | `u_BRDFLUT` | 11 | Fragment | `RendererBindings::BRDFLUT` |
| UniformBuffer | `DeferredPassBlock` | 1 | Fragment | `RendererBindings::PassBlock` |
| UniformBuffer | `LightBlock` | 0 | Fragment | `RendererBindings::LightBlock` |

`DeferredPassBlock.u_DeferredParams` 当前按 `x/y/z/w` 保存 RT 采样 Y 翻转标志、`ERenderDebugView`、当前后端是否使用 `[0,1]` NDC 深度以及保留值。`u_InvViewProjection` 是 Renderer Reversed-Z 转换、再经后端调整后的 `Projection * View` 的逆矩阵；正式 Lighting 与 `WorldPositionReconstructionError` 都直接使用采样深度和该逆矩阵重建世界坐标，无需先恢复普通 Z。`u_GBufferWorldPosition` 暂时继续绑定，仅供存储位置和重建误差调试视图采样。

### `sky.frag`

| 类型 | 名称 | Binding | Stage | C++ 对应 |
| --- | --- | ---: | --- | --- |
| UniformBuffer | `SkyBlock` | 1 | Fragment | `RendererBindings::PassBlock` |
| TextureCube | `u_PrefilterMap` | 10 | Fragment | `RendererBindings::PrefilterMap` |

## PipelineLayout 对照

### StaticMesh BasePass Pipeline

构建位置：`FRenderResourceManager::BuildStaticMeshBasePassPipeline`

| Group | Binding 内容 | 资源类型 | Stage |
| ---: | --- | --- | --- |
| 0 | `LightBlock` binding 0 | UniformBuffer | Fragment |
| 1 | `ObjectBlock` binding 1 | UniformBuffer | Vertex |
| 2 | `BaseColor/Normal/Metallic/Roughness/AO/Emissive` binding 2..7 | Texture2D | Fragment |
| 3 | `MaterialBlock` binding 8 | UniformBuffer | Fragment |
| 4 | `IrradianceMap/PrefilterMap/BRDFLUT` binding 9..11 | TextureCube / Texture2D | Fragment |

### Deferred GBuffer Pipeline

构建位置：`FDeferredRenderPath::BuildGBufferPipeline`

| Group | Binding 内容 | 资源类型 | Stage |
| ---: | --- | --- | --- |
| 1 | `ObjectBlock` binding 1 | UniformBuffer | Vertex |
| 2 | `BaseColor/Normal/Metallic/Roughness/AO/Emissive` binding 2..7 | Texture2D | Fragment |
| 3 | `MaterialBlock` binding 8 | UniformBuffer | Fragment |

### Deferred Lighting Pipeline

构建位置：`FDeferredRenderPath::BuildLightingPipeline`

| Group | Binding 内容 | 资源类型 | Stage |
| ---: | --- | --- | --- |
| 0 | `LightBlock` binding 0 | UniformBuffer | Fragment |
| 1 | `DeferredPassBlock` binding 1 | UniformBuffer | Fragment |
| 2 | `GBufferAlbedo/Normal/WorldPosition/Depth/Material` binding 2..6 | Texture2D | Fragment |
| 4 | `IrradianceMap/PrefilterMap/BRDFLUT` binding 9..11 | TextureCube / Texture2D | Fragment |

### Sky Pipeline

构建位置：`FForwardRenderPath::BuildSkyPipeline`

| Group | Binding 内容 | 资源类型 | Stage |
| ---: | --- | --- | --- |
| 1 | `SkyBlock` binding 1 | UniformBuffer | Fragment |
| 4 | `IrradianceMap/PrefilterMap/BRDFLUT` binding 9..11 | TextureCube / Texture2D | Fragment |

注意：`sky.frag` 当前只采样 `u_PrefilterMap` binding 10，但 C++ 侧 `Sky_EnvironmentTextures_Layout` 仍使用完整 Environment layout，包含 binding 9、10、11。这是当前允许的冗余绑定，后续校验工具应区分“shader 必需资源缺失”和“layout 包含 shader 未使用资源”。

## BindGroup 创建位置

| 资源组 | 创建位置 | 当前说明 |
| --- | --- | --- |
| LightBlock | `RendererLightUniforms.cpp` | 创建光源 UBO 与 group 0 |
| ObjectBlock | `RendererPassUniforms.cpp` | 使用 `PassBlock` binding 1 / group 1 |
| DeferredPassBlock | `RendererPassUniforms.cpp` | 使用 `PassBlock` binding 1 / group 1 |
| SkyBlock | `RendererPassUniforms.cpp` | 使用 `PassBlock` binding 1 / group 1 |
| MaterialBlock | `RendererPassUniforms.cpp` | 使用 `MaterialBlock` binding 8 / group 3 |
| MaterialTextures | `RendererTextureBindings.cpp` | 创建材质贴图 group 2 |
| GBufferTextures | `RendererTextureBindings.cpp` | 创建 Deferred Lighting GBuffer 输入 group 2 |
| Environment | `RendererTextureBindings.cpp` | 创建 IBL 环境资源 group 4 |

## 阶段一结论

当前绑定体系可以继续支撑 OpenGL 单后端运行，但已经存在后续自动化必须处理的约束：

1. `PassBlock` binding 1 在不同 pipeline 中代表 `ObjectBlock`、`DeferredPassBlock` 或 `SkyBlock`，需要按 pipeline 语境解释。
2. `MaterialTextures` 与 `GBufferTextures` 共享 group index 2，需要按 pipeline 语境隔离。
3. Environment layout 在 Sky Pipeline 中存在冗余资源声明，校验工具不能简单要求 layout 与 shader 使用资源完全相等。
4. OpenGL GLSL 当前只有 `binding`，没有显式 `set` / `group`，后续如果要对齐 Vulkan，需要新增 group 元数据约定或切到 SPIR-V 反射。
5. `RendererBindings` 的命名与 shader 变量名并非完全一致，生成工具需要一套稳定的命名映射规则。

阶段二应在本文清单基础上实现最小 GLSL 扫描或解析校验，优先检查：

- shader 中声明的 binding 是否都能在对应 PipelineLayout 中找到。
- C++ layout 中的资源类型是否与 shader 类型一致。
- stage visibility 是否覆盖 shader 实际使用阶段。
- vertex input location 是否与 `FStaticMeshVertex` 的 Pipeline vertex input 描述一致。
