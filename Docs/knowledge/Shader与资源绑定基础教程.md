# Shader 与资源绑定基础教程

本文面向刚开始阅读渲染代码的开发者，解释 shader、RHI 资源绑定、binding、layout、descriptor、uniform、sampler 等概念之间的关系。目标是：以后看到一段 VS / FS shader 时，能判断它需要哪些输入、会输出什么、CPU 侧必须准备什么资源，以及 `layout(binding = N)` 这类语句到底在做什么。

本文只描述基础概念和 ToyEngine 当前实现方式，不代表项目已经具备完整 Vulkan / D3D12 后端或自动 shader 反射系统。

## 一句话理解 Shader 与资源绑定

shader 是运行在 GPU 上的小程序。CPU 不能像调用普通 C++ 函数那样直接把参数塞进 shader 的局部变量，只能通过图形 API 约定好的方式把资源放到某些“槽位”上，然后 GPU 执行 shader 时再从这些槽位读取。

因此渲染代码至少要回答三个问题：

1. shader 需要什么输入。
2. CPU / RHI 把这些输入放到哪里。
3. shader 和 CPU 是否对“哪里”有同一个理解。

`layout(binding = 2) uniform sampler2D u_BaseColorTex;` 这句话就在回答第三个问题：这个 fragment shader 有一张 2D 纹理输入，shader 内名字叫 `u_BaseColorTex`，它希望 CPU 把对应纹理绑定到 binding slot 2。

## 为什么需要复杂的资源绑定

一次 draw call 不是只执行一行 shader。它通常需要很多数据：

- 顶点数据：位置、法线、UV、颜色、切线。
- 每个物体的数据：模型矩阵、MVP 矩阵、法线矩阵。
- 每帧或每个 pass 的数据：相机位置、视图投影矩阵、调试模式。
- 材质数据：BaseColor、Metallic、Roughness、AO、Emissive 参数。
- 材质贴图：BaseColor 贴图、Normal 贴图、Metallic 贴图等。
- 光源数据：方向光、点光位置和颜色。
- 环境资源：irradiance cubemap、prefilter cubemap、BRDF LUT。
- Deferred 渲染的中间结果：GBuffer 里的 Albedo、Normal、WorldPosition、Depth、Material。

这些数据类型不同、更新频率不同、生命周期不同、属于不同渲染阶段。把它们全部塞成一个“大结构”既不现实，也不适合现代图形 API。因此渲染系统会把资源分组、分槽、分 layout 描述，然后在 draw 前绑定。

ToyEngine 当前已经有 `BindGroupLayout` / `PipelineLayout`，就是为了把这些 GPU 资源接口显式描述出来。

## 从 C++ 到 Shader 的数据流

一次静态网格绘制可以简化成下面这样：

```text
C++ / Engine / Renderer
  准备 Mesh 顶点缓冲
  准备 ObjectBlock UBO
  准备 LightBlock UBO
  准备 MaterialBlock UBO
  准备材质贴图和采样器
  准备 Environment 贴图
  创建 PipelineLayout
  创建 BindGroup
  cmdBuf->SetPipeline(...)
  cmdBuf->SetBindGroup(...)
  cmdBuf->DrawIndexed(...)

GPU
  对每个顶点执行 Vertex Shader
  光栅化生成片元
  对每个片元执行 Fragment Shader
  写入颜色 RT / GBuffer / 默认帧缓冲
```

shader 只能读取 GPU 当前绑定好的资源。如果 CPU 忘记绑定某张纹理，或者把纹理绑到了错误 binding，shader 里的 `texture(u_BaseColorTex, vTexCoord)` 就会读错资源，轻则画面错误，重则 API 报错或结果未定义。

## 核心术语

### Shader

shader 是 GPU 程序。常见阶段包括：

- Vertex Shader，简称 VS：处理每个顶点。
- Fragment Shader，OpenGL 中常叫 FS，D3D 中常叫 Pixel Shader / PS：处理每个像素片元。
- Geometry / Tessellation / Compute Shader：更高级阶段，当前 ToyEngine 主路径暂不重点讨论。

### Vertex Shader

VS 的核心职责是把模型顶点变换到裁剪空间，并把后续 FS 需要的数据传下去。

典型输入：

- `layout(location = 0) in vec3 aPosition;`
- `layout(location = 1) in vec3 aNormal;`
- `layout(location = 2) in vec2 aTexCoord;`

典型输出：

- `gl_Position`
- `out vec3 vWorldNormal;`
- `out vec2 vTexCoord;`

`gl_Position` 是固定内建输出，必须写。GPU 后续会用它做裁剪、透视除法、视口变换和光栅化。

### Fragment Shader

FS 的核心职责是计算当前片元最终输出什么。它可以输出最终颜色，也可以输出 GBuffer 中间数据。

典型输入：

- VS 传下来的插值数据，例如 `in vec2 vTexCoord;`
- uniform buffer，例如 `LightBlock`、`MaterialBlock`
- texture / sampler，例如 `sampler2D u_BaseColorTex`

典型输出：

- Forward 渲染：`out vec4 fragColor;`
- Deferred GBuffer：多个输出，例如 `outAlbedo`、`outNormal`、`outMaterial`

### Uniform

uniform 是 shader 中“对一次 draw 或一批 draw 保持相同”的数据。比如一个物体的模型矩阵、一个材质参数、一组光源数据。

单个 uniform 可以理解为 shader 的全局只读参数。现代图形 API 更常把多个 uniform 打包进 uniform buffer。

### Uniform Buffer / UBO

UBO 是一块 GPU buffer，里面放一组 uniform 数据。ToyEngine 中的 `ObjectBlock`、`LightBlock`、`MaterialBlock` 都是 UBO。

例如：

```glsl
layout(std140, binding = 1) uniform ObjectBlock {
    mat4 u_MVP;
    mat4 u_Model;
    mat4 u_NormalMatrix;
};
```

含义是：

- 这是一个 uniform block，名字叫 `ObjectBlock`。
- 内存布局使用 `std140` 规则。
- shader 希望 CPU 把这个 UBO 绑定到 binding 1。
- shader 里面可以读取 `u_MVP`、`u_Model`、`u_NormalMatrix`。

### 为什么每个 Draw 需要独立 Uniform 范围

OpenGL 的 Draw 通常立即读取当时绑定的 Buffer 内容，所以“更新同一个 UBO，然后 Draw，再更新同一个 UBO”看起来能够工作。Vulkan 等录制式 API 只把 Buffer 及其范围写进 CommandBuffer，GPU 可能在 CPU 完成整帧录制后才读取；如果所有 Draw 都引用同一范围，它们就可能一起看到最后一次写入的数据。

ToyEngine 当前使用按帧分段的 transient Uniform ring 解决这个问题：

```text
frame segment
  offset 0    -> draw A 的 ObjectBlock
  offset 256  -> draw B 的 ObjectBlock
  offset 512  -> MaterialBlock
```

BindGroup 持有稳定的 ring buffer，`SetBindGroup` 额外传入 dynamic offset，后端只绑定当前数据范围。分配器保证 offset 满足 Uniform Buffer 对齐，并且不同 frames-in-flight 使用不同 segment；只有对应帧已经安全回收时才重置该 segment。

这个方案表达的是“录制命令必须引用稳定数据快照”。它不仅是性能优化，也是从立即执行 API 迁移到显式 API 时的正确性要求。

### Texture

Texture 是 GPU 纹理资源，保存图片、法线、GBuffer、BRDF LUT、cubemap 等数据。shader 不能直接读取 C++ 图片对象，而是读取图形 API 创建的 GPU texture。

### Sampler

Sampler 描述如何采样纹理，例如：

- 放大/缩小时用 nearest 还是 linear。
- 是否使用 mipmap。
- UV 超过 0..1 时 repeat 还是 clamp。
- 是否启用各向异性过滤。

OpenGL GLSL 中 `sampler2D` 经常把 texture 和 sampler 的概念合在 shader 类型里。RHI / Vulkan / D3D12 层则可能把 texture view 和 sampler 拆得更明确。

### Binding

binding 是 shader 资源槽位编号。

```glsl
layout(binding = 2) uniform sampler2D u_BaseColorTex;
```

表示 `u_BaseColorTex` 要从 binding 2 读取。

C++ 侧必须创建对应的 bind group entry：

```text
binding = 2
type = Texture2D
texture = BaseColorTexture
sampler = MaterialSampler
```

如果 shader 认为 BaseColor 在 binding 2，但 C++ 把 Normal 贴图放到 binding 2，结果就会把法线图当颜色读。

### Location

location 是顶点输入、shader 阶段间输出输入、fragment 输出的编号。

```glsl
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
```

在 VS 输入里，location 对应 pipeline vertex input 描述。C++ 必须告诉 GPU：

```text
location 0 -> 顶点结构中的 Position
location 1 -> 顶点结构中的 Normal
location 2 -> 顶点结构中的 TexCoord
```

在 FS 输出里，location 对应 render target attachment：

```glsl
layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
```

表示输出 0 写到颜色附件 0，输出 1 写到颜色附件 1。

### Layout

layout 是 shader 里用来声明接口布局的关键字。常见用途：

```glsl
layout(location = 0) in vec3 aPosition;
layout(location = 0) out vec4 outAlbedo;
layout(binding = 2) uniform sampler2D u_BaseColorTex;
layout(std140, binding = 1) uniform ObjectBlock { ... };
```

它不是“创建资源”，而是告诉编译器和图形 API：这个 shader 接口要使用哪些编号和内存规则。

### Descriptor

descriptor 是现代图形 API 对“资源绑定描述”的称呼。可以把 descriptor 理解为 GPU 可读的资源名片：

```text
这个槽位是一个 Texture2D
实际资源是 BaseColorTexture
采样器是 LinearWrapSampler
shader 阶段是 Fragment
```

Vulkan 叫 descriptor / descriptor set，D3D12 叫 descriptor heap / root signature / descriptor table，WebGPU 叫 bind group。ToyEngine 的 RHI 用更通用的名字：

- `RHIBindGroupEntry`：一条资源绑定。
- `RHIBindGroupLayoutEntry`：一条资源绑定应该长什么样。
- `RHIBindGroup`：一组实际资源。
- `RHIBindGroupLayout`：一组资源的结构描述。
- `RHIPipelineLayout`：一个 pipeline 接受哪些 bind group。

### BindGroup

BindGroup 是“一组已经填好实际 GPU 资源的绑定”。例如材质贴图 BindGroup：

```text
binding 2 -> BaseColor Texture2D
binding 3 -> Normal Texture2D
binding 4 -> Metallic Texture2D
binding 5 -> Roughness Texture2D
binding 6 -> AO Texture2D
binding 7 -> Emissive Texture2D
```

绘制前调用：

```cpp
cmdBuf->SetBindGroup(RendererBindGroups::MaterialTextures, state.BindGroup.get());
```

意思是把这组材质贴图绑定到当前 pipeline 的 `MaterialTextures` group。

### BindGroupLayout

BindGroupLayout 描述一个 BindGroup 应该包含哪些槽位和类型，但不包含实际资源。

例如：

```text
binding 2 必须是 Texture2D，Fragment 可见
binding 3 必须是 Texture2D，Fragment 可见
binding 4 必须是 Texture2D，Fragment 可见
```

它像函数签名，BindGroup 像函数调用时传入的实参。

### PipelineLayout

PipelineLayout 描述一个 shader pipeline 总共接受哪些 bind group。

例如 StaticMesh BasePass Pipeline 当前可以理解为：

```text
group 0 -> LightBlock layout
group 1 -> ObjectBlock layout
group 2 -> MaterialTextures layout
group 3 -> MaterialBlock layout
group 4 -> Environment layout
```

创建 pipeline 时，RHI 后端会用它确认 shader 资源接口和 CPU 侧绑定模型是否能对应。

### Group / Set

group 是一组资源的编号。Vulkan 中对应 descriptor set，WebGPU 中也叫 bind group。

ToyEngine 当前有：

```text
group 0: LightBlock
group 1: PassBlock
group 2: MaterialTextures 或 GBufferTextures
group 3: MaterialBlock
group 4: Environment
```

ToyEngine 的共享 GLSL 当前使用 `TE_RESOURCE_BINDING(group, binding)` 和 `TE_UNIFORM_BINDING(group, binding)`。OpenGL 运行时展开为 `layout(binding = N)`；Vulkan SPIR-V 编译展开为 `layout(set = group, binding = N)`。因此 group/set 已进入 shader 源级声明，但 C++ 的 PipelineLayout 仍需手工与它保持一致，自动反射与校验尚未完成。

## Shader 中的几类输入输出

### 顶点属性输入

```glsl
layout(location = 0) in vec3 aPosition;
```

这是从 vertex buffer 读取的 per-vertex 数据。每个顶点不同。

### 阶段间插值输入输出

VS 中：

```glsl
out vec2 vTexCoord;
```

FS 中：

```glsl
in vec2 vTexCoord;
```

VS 为三角形三个顶点输出 UV，光栅化阶段会对三角形内部每个片元插值，FS 读到的是当前像素位置对应的插值 UV。

### Uniform Block

```glsl
layout(std140, binding = 8) uniform MaterialBlock {
    vec4 u_BaseColorFactor_Metallic;
};
```

这是 CPU 通过 UBO 上传的一组参数。一次 draw 内通常不随像素变化。

### Texture Sampler

```glsl
layout(binding = 2) uniform sampler2D u_BaseColorTex;
vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb;
```

这是用 UV 坐标从 2D 纹理采样。

### Fragment Output

```glsl
out vec4 fragColor;
```

或：

```glsl
layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
```

这是 FS 写出的结果。Forward 通常写最终颜色，Deferred GBuffer Pass 通常写多个中间纹理。

## 看 Shader 的推荐顺序

以后看到一个 shader，不要从数学公式开始硬读。先按下面顺序扫：

1. 看 `#version`：知道 GLSL 版本和语法基线。
2. 看 `layout(location = N) in`：它从顶点或上一阶段拿什么。
3. 看 `in / out`：它和前后阶段传什么。
4. 看 `layout(binding = N)`：它需要 CPU 绑定哪些资源。
5. 看 uniform block：它需要哪些常量参数。
6. 看 texture / sampler：它采样哪些纹理。
7. 看 `main()`：它最终写什么输出。
8. 最后再看辅助函数：这些通常是光照、法线、颜色空间、调试模式等算法细节。

## ToyEngine 的 `model.vert` 逐段解释

```glsl
#version 450 core
```

使用 GLSL 4.5 Core 语法。

```glsl
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec3 aTangent;
```

这些是顶点输入。C++ 的 pipeline vertex input 必须保证：

- location 0 读 `FStaticMeshVertex::Position`
- location 1 读 `FStaticMeshVertex::Normal`
- location 2 读 `FStaticMeshVertex::TexCoord`
- location 3 读 `FStaticMeshVertex::Color`
- location 4 读 `FStaticMeshVertex::Tangent`

```glsl
layout(std140, binding = 1) uniform ObjectBlock {
    mat4 u_MVP;
    mat4 u_Model;
    mat4 u_NormalMatrix;
};
```

这是每个物体的 UBO。`binding = 1` 对应 C++ 的 `RendererBindings::PassBlock`。这里放：

- `u_MVP`：把模型顶点变换到裁剪空间。
- `u_Model`：把模型顶点变换到世界空间。
- `u_NormalMatrix`：把法线和切线变换到世界空间。

```glsl
out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec3 vWorldTangent;
out vec2 vTexCoord;
out vec3 vColor;
```

这些会传给 FS，并在三角形内部自动插值。

```glsl
vec4 worldPosition = u_Model * vec4(aPosition, 1.0);
gl_Position = u_MVP * vec4(aPosition, 1.0);
```

第一行算世界坐标，供光照使用。第二行写 `gl_Position`，供 GPU 后续光栅化使用。

```glsl
vWorldNormal = normalize((u_NormalMatrix * vec4(aNormal, 0.0)).xyz);
vWorldTangent = normalize((u_NormalMatrix * vec4(aTangent, 0.0)).xyz);
```

把模型空间法线和切线变成世界空间方向。`w = 0.0` 表示这是方向向量，不受平移影响。

```glsl
vTexCoord = aTexCoord;
vColor = aColor;
```

把 UV 和顶点色传给 FS。

## ToyEngine 的 Forward `model.frag` 逐段解释

这个 FS 负责 Forward PBR：直接在一个 shader 中采样材质、计算法线、直接光、IBL 环境光，最后输出颜色。

```glsl
in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vWorldTangent;
in vec2 vTexCoord;
in vec3 vColor;
```

这些来自 VS，是当前片元插值后的世界位置、法线、切线、UV、顶点色。

```glsl
layout(std140, binding = 0) uniform LightBlock { ... };
```

光源 UBO，binding 0。包含方向光数量、点光数量、方向、颜色、位置、半径等。

```glsl
layout(std140, binding = 8) uniform MaterialBlock { ... };
```

材质参数 UBO，binding 8。包含 BaseColor factor、Metallic factor、Roughness factor、AO factor、Emissive factor、相机位置。

```glsl
layout(binding = 2) uniform sampler2D u_BaseColorTex;
layout(binding = 3) uniform sampler2D u_NormalTex;
layout(binding = 4) uniform sampler2D u_MetallicTex;
layout(binding = 5) uniform sampler2D u_RoughnessTex;
layout(binding = 6) uniform sampler2D u_AOTex;
layout(binding = 7) uniform sampler2D u_EmissiveTex;
```

材质贴图。CPU 侧必须在材质贴图 BindGroup 中把这些贴图绑定到 2 到 7。

```glsl
layout(binding = 9) uniform samplerCube u_IrradianceMap;
layout(binding = 10) uniform samplerCube u_PrefilterMap;
layout(binding = 11) uniform sampler2D u_BRDFLUT;
```

IBL 环境光资源：

- `u_IrradianceMap`：漫反射环境光。
- `u_PrefilterMap`：粗糙度相关的镜面环境反射。
- `u_BRDFLUT`：预积分 BRDF 查找表。

```glsl
out vec4 fragColor;
```

最终输出颜色。

```glsl
vec3 GetMaterialNormal()
```

这个函数把 normal map 从切线空间变换到世界空间：

1. 取插值后的世界法线 `n`。
2. 正交化世界切线 `t`。
3. 用 `cross(n, t)` 算副切线 `b`。
4. 组成 TBN 矩阵。
5. 从 normal texture 采样，把 0..1 转成 -1..1。
6. 用 TBN 把切线空间法线转到世界空间。

```glsl
DistributionGGX
GeometrySchlickGGX
GeometrySmith
FresnelSchlick
FresnelSchlickRoughness
EvaluatePBRLight
```

这些是 PBR 光照函数。阅读时先知道它们共同服务一个目标：根据法线、视线、光线、材质参数计算直接光贡献。细节属于 BRDF 数学。

```glsl
vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb * u_BaseColorFactor_Metallic.rgb * vColor;
```

采样 BaseColor 贴图，乘材质颜色因子，再乘顶点色，得到最终基础色。

```glsl
float metallic = clamp(texture(u_MetallicTex, vTexCoord).r * u_BaseColorFactor_Metallic.w, 0.0, 1.0);
float roughness = clamp(texture(u_RoughnessTex, vTexCoord).r * u_RoughnessAOEmissiveStrength_Pad.x, 0.04, 1.0);
float ao = clamp(texture(u_AOTex, vTexCoord).r * u_RoughnessAOEmissiveStrength_Pad.y, 0.0, 1.0);
```

从贴图和 factor 得到金属度、粗糙度、AO。`clamp` 保证数值在合理范围。

```glsl
vec3 n = GetMaterialNormal();
vec3 v = normalize(u_CameraPosition_Pad.xyz - vWorldPosition);
```

`n` 是当前像素世界空间法线，`v` 是从像素指向相机的视线方向。

```glsl
vec3 irradiance = texture(u_IrradianceMap, n).rgb;
vec3 diffuseIBL = irradiance * baseColor;
```

根据法线方向采样环境漫反射。

```glsl
vec3 reflection = reflect(-v, n);
vec3 prefilteredColor = textureLod(u_PrefilterMap, reflection, roughness * 4.0).rgb;
vec2 brdf = texture(u_BRDFLUT, vec2(nDotV, roughness)).rg;
```

根据反射方向和粗糙度采样环境镜面反射，并用 BRDF LUT 修正。

```glsl
for (int i = 0; i < u_LightCounts.x; ++i)
```

遍历方向光。

```glsl
for (int i = 0; i < u_LightCounts.y; ++i)
```

遍历点光，计算距离衰减，再累加 PBR 直接光。

```glsl
fragColor = vec4(color + emissive, 1.0);
```

输出最终颜色。`emissive` 直接加到结果中。

## ToyEngine 的 Deferred GBuffer `gbuffer.frag`

这个 shader 不计算最终光照。它把材质和几何信息写入多个 render target，供后面的 Deferred Lighting Pass 使用。

```glsl
layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outWorldPosition;
layout(location = 3) out vec4 outMaterial;
```

它有 4 个输出：

- `outAlbedo`：基础色。
- `outNormal`：编码后的世界空间法线。
- `outWorldPosition`：世界坐标。
- `outMaterial`：metallic、roughness、ao、emissiveLuma。

```glsl
vec3 encodedNormal = GetMaterialNormal() * 0.5 + 0.5;
```

法线原本是 -1..1，写入普通颜色纹理前编码到 0..1。

```glsl
outMaterial = vec4(metallic, roughness, ao, emissiveLuma);
```

把多个标量材质参数打包到一张 GBuffer 纹理的 RGBA 通道。

Deferred 的关键思想是：第一个 pass 只写“之后算光需要的信息”，第二个 pass 再统一读 GBuffer 算光照。

## ToyEngine 的 Deferred Lighting `deferred_lighting.frag`

这个 shader 读取 GBuffer，恢复出材质和几何信息，然后计算光照。

```glsl
in vec2 vScreenUV;
```

这是全屏三角形传下来的屏幕 UV。它不是模型 UV，而是当前屏幕像素位置对应的 UV。

```glsl
layout(binding = 2) uniform sampler2D u_GBufferAlbedo;
layout(binding = 3) uniform sampler2D u_GBufferNormal;
layout(binding = 4) uniform sampler2D u_GBufferWorldPosition;
layout(binding = 5) uniform sampler2D u_GBufferDepth;
layout(binding = 6) uniform sampler2D u_GBufferMaterial;
```

这些是 GBuffer 输入。C++ 侧 Deferred Lighting pass 必须把上一阶段生成的 render target 绑定到这些槽位。

```glsl
layout(std140, binding = 1) uniform DeferredPassBlock {
    ivec4 u_DeferredParams;
    vec4 u_CameraPosition_Pad;
    mat4 u_InvViewProjection;
};
```

Deferred pass 参数。`u_DeferredParams.x` 是 RT 采样是否翻转，`.y` 是调试视图模式，`.z` 表示后端 NDC 深度是否为 `[0,1]`；其余字段保存相机位置和后端调整后 ViewProjection 的逆矩阵。

```glsl
if (u_DeferredParams.x != 0)
{
    uv.y = 1.0 - uv.y;
}
```

处理不同后端或 render target 采样坐标方向差异。

```glsl
float depth = texture(u_GBufferDepth, uv).r;
if (depth <= 0.000001)
{
    fragColor = vec4(GetSkyColor(uv), 1.0);
    return;
}
```

当前主渲染路径使用 Reversed-Z，Near=1、Far=0。如果深度接近 0，认为这个像素没有几何体或位于远平面，直接画天空。Reversed-Z 不会让深度线性化；它只是反转深度方向，以利用 `D32_Float` 在 0 附近更密集的浮点分布改善远处精度。

```glsl
vec3 baseColor = texture(u_GBufferAlbedo, uv).rgb;
vec3 normal = normalize(texture(u_GBufferNormal, uv).rgb * 2.0 - 1.0);
vec4 material = texture(u_GBufferMaterial, uv);
vec3 worldPosition = ReconstructWorldPosition(uv, depth);
```

从 GBuffer 还原光照所需数据。法线从 0..1 解码回 -1..1，世界坐标则由 Reversed-Z 深度和 inverse view-projection 重建。正式 PBR 的视线向量、点光向量、距离与衰减统一使用该重建位置。

`WorldPositionReconstructionError` 调试视图会把屏幕 UV 和深度转换回后端 NDC，再乘 `u_InvViewProjection` 重建世界坐标，并与 `u_GBufferWorldPosition` 比较。Position RT 目前只为这类对照调试保留，不再作为正式 Deferred 光照输入。黑色表示 `<1 μm`，蓝色表示 `[1,10) μm`，青色表示 `[10,100) μm`，绿色表示 `[0.1,1) mm`，黄色表示 `[1,10) mm`，红色表示 `≥1 cm`。

```glsl
if (u_DeferredParams.y == 1)
{
    fragColor = vec4(baseColor, 1.0);
    return;
}
```

调试视图：如果请求显示 BaseColor，就提前输出 BaseColor，不继续算光。

后面的 PBR / IBL / light loop 和 Forward shader 类似，只是输入来自 GBuffer，而不是直接来自材质贴图和 VS 插值。

## VS 和 FS 的关系

VS 和 FS 中同名的 `out` / `in` 是一条数据通路：

```glsl
// VS
out vec2 vTexCoord;

// FS
in vec2 vTexCoord;
```

VS 对每个顶点输出值，FS 对每个片元读取插值后的值。比如一个三角形三个顶点的 UV 分别是 `(0,0)`、`(1,0)`、`(0,1)`，三角形中间像素读到的 UV 会是插值结果。

这也是为什么 FS 能用 `vTexCoord` 采样贴图：它不是每个像素手动传来的，而是 GPU 光栅化阶段自动插值得到的。

## CPU 侧 RHI 与 Shader 的对应关系

以 `model.frag` 的 BaseColor 贴图为例：

shader 写：

```glsl
layout(binding = 2) uniform sampler2D u_BaseColorTex;
```

C++ 常量写：

```cpp
constexpr uint32_t BaseColorTexture = 2;
```

BindGroupLayout 写：

```text
binding 2, Texture2D, Fragment
```

BindGroup 写：

```text
binding 2, Texture2D, texture = 当前材质 BaseColor, sampler = 材质采样器
```

Draw 前绑定：

```text
SetBindGroup(MaterialTextures group, 当前材质 BindGroup)
```

最终 GPU 执行：

```glsl
texture(u_BaseColorTex, vTexCoord)
```

这整条链必须一致。任何一处不一致都会导致 shader 读错。

## 为什么 C++ 还要写 Layout

因为现代图形 API 希望在真正 draw 之前就知道 pipeline 需要什么资源结构。这样后端可以：

- 提前验证资源类型是否匹配。
- 提前创建 Vulkan `VkPipelineLayout` 或 D3D12 Root Signature。
- 避免 draw 时才临时猜 shader 需要什么。
- 让多后端有统一抽象。

Shader 里的 set/binding 宏是 shader 侧声明。C++ 的 `BindGroupLayout` 是 CPU / RHI 侧声明。理想状态是两者来自同一个真相源；当前 ToyEngine 还没有自动反射，所以先通过共享宏让 OpenGL/Vulkan 使用同一份 shader 声明，后续再做 C++ layout 校验和生成。

## 常见关键字速查

| 关键字 | 大致含义 | 常见位置 |
| --- | --- | --- |
| `#version` | GLSL 版本 | 文件开头 |
| `in` | 当前 shader 输入 | VS 输入顶点属性，FS 输入插值数据 |
| `out` | 当前 shader 输出 | VS 输出插值数据，FS 输出颜色/RT |
| `uniform` | CPU 提供的只读参数或资源 | UBO、texture sampler |
| `layout(location = N)` | 输入输出编号 | 顶点属性、fragment output |
| `layout(binding = N)` | 资源绑定槽位 | UBO、sampler |
| `layout(std140)` | UBO 内存布局规则 | uniform block |
| `sampler2D` | 2D 纹理采样器 | 材质贴图、GBuffer |
| `samplerCube` | Cubemap 采样器 | 天空、IBL |
| `texture(...)` | 采样纹理 | FS 中最常见 |
| `textureLod(...)` | 指定 mip level 采样 | IBL、特殊采样 |
| `normalize` | 归一化向量 | 法线、方向 |
| `dot` | 点乘 | 光照角度、投影 |
| `cross` | 叉乘 | 构造切线空间 |
| `reflect` | 反射方向 | 镜面反射、IBL |
| `clamp` | 限制数值范围 | 材质参数、衰减 |
| `mix` | 线性插值 | 金属/非金属 F0 混合 |
| `discard` | 丢弃片元 | Alpha test，当前主 shader 未用 |
| `gl_Position` | VS 必写裁剪空间位置 | Vertex Shader |
| `gl_VertexID` | 当前顶点 ID | 全屏三角形常用 |

## 看到陌生 Shader 时怎么判断它在干嘛

可以用下面清单快速判断：

1. 文件名和 pipeline 名：是 model、gbuffer、lighting、sky、postprocess 还是 shadow。
2. Stage：VS 通常写 `gl_Position`，FS 通常写颜色或 GBuffer。
3. 输入：有没有顶点属性、屏幕 UV、世界坐标、法线。
4. 资源：有哪些 UBO、纹理、cubemap。
5. 输出：是一个 `fragColor`，还是多个 `layout(location) out`。
6. 坐标空间：变量名里有没有 `World`、`View`、`Clip`、`Tangent`。
7. 是否采样材质贴图：找 `texture(u_BaseColorTex, ...)` 这类语句。
8. 是否算光：找 light loop、BRDF、dot(normal, light)、Fresnel、GGX。
9. 是否是 Deferred：如果读取 GBuffer，就是 Lighting Pass；如果写多个 GBuffer，就是 GBuffer Pass。
10. 是否是后处理：如果输入是屏幕 UV，输出一个颜色，通常是全屏 pass。

## 常见错误与排查方向

### 画面全黑

优先检查：

- FS 是否写了输出。
- Pipeline 是否绑定了正确 shader。
- 必需 UBO / texture 是否绑定。
- 光源数量是否为 0。
- BaseColor / AO / Roughness 是否读错通道。

### 贴图看起来错乱

优先检查：

- UV 是否传对。
- binding 是否把错误纹理绑到了 shader。
- sRGB / Linear 是否处理正确。
- sampler 是否使用了错误 wrap 或 filter。

### 法线方向奇怪

优先检查：

- Normal Matrix 是否正确。
- Tangent 是否导入或生成。
- TBN 是否正交化。
- normal map 是否按线性数据读取。
- 法线从 0..1 到 -1..1 的转换是否正确。

### Deferred 结果和 Forward 不一致

优先检查：

- GBuffer 写入和 Lighting Pass 读取的通道是否一致。
- 法线编码/解码是否一致。
- world position 是否正确。
- GBuffer sampler 是否应使用 nearest。
- RT 采样 Y 翻转是否正确。

### Binding 改了之后不生效

优先检查：

- shader `layout(binding = N)` 是否改了。
- `RendererBindings` 是否同步。
- `RHIBindGroupLayoutDesc` 是否同步。
- `RHIBindGroupDesc` 实际资源是否同步。
- PipelineLayout 是否包含对应 group。

## ToyEngine 当前资源绑定心智模型

当前可以先记住这张简化表：

```text
Shader layout(binding = N)
        |
        v
RendererBindings 中的常量
        |
        v
RHIBindGroupLayoutDesc 声明这个 binding 应该是什么类型
        |
        v
RHIBindGroupDesc 把真实 GPU 资源填进去
        |
        v
cmdBuf->SetBindGroup(groupIndex, bindGroup)
        |
        v
GPU 执行 shader 时从 binding N 读取资源
```

其中 `groupIndex` 由 C++ 的 `RendererBindGroups` 管理，并由 shader 公共宏映射为 Vulkan descriptor set；`binding N` 由 shader 和 C++ 常量共同约定。当前项目还没有自动验证两者一致性，所以仍需要 `Shader资源绑定清单` 和后续反射/校验流程。

## 推荐继续阅读

- `Docs/reference/Shader资源绑定清单.md`：当前 ToyEngine shader 与 C++ 绑定的实际清单。
- `Docs/architecture/Shader反射与资源绑定描述推进规划.md`：后续如何从手写绑定推进到校验和反射。
- `Docs/guides/渲染管线.md`：当前 Renderer、Forward、Deferred、RHI 的主流程。
- `Docs/knowledge/PBR材质与颜色空间基础.md`：材质参数、贴图语义、sRGB / Linear 的基础。
