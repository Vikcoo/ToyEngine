# IBL 环境光照原理与实现

本文档是 ToyEngine 的 IBL（Image-Based Lighting，基于图像的光照）专题知识记录，同时作为 [`引擎学习路径与知识脉络`](./引擎学习路径与知识脉络.md) 中「从原理到实现」的样板文档。

阅读顺序建议：先看「一、为什么需要 IBL」到「四、Specular 的分割求和近似」建立理论基础，再看「五、本项目的实现」把每个公式对应到具体代码行，最后看「七、当前偏差与已知限制」了解本项目实现与标准做法的差距。

前置知识：[`PBR 材质与颜色空间基础`](./PBR材质与颜色空间基础.md) 中的 Linear/sRGB 约定与材质参数语义。

---

## 一、为什么需要 IBL

### 直接光照解决不了什么

当前 Forward / Deferred 路径都支持方向光和点光。它们回答的是「有几个明确光源照过来」，对应渲染方程中来自少数离散方向的贡献。

现实中并非如此。站在户外时，进入眼睛的光绝大部分不是直接来自太阳，而是来自整个天空穹顶的散射光、地面的反射光和周围物体的间接反射。如果只算直接光，结果是：

- 背光面完全漆黑，因为没有任何光到达
- 金属材质几乎不可见，因为金属的漫反射为零，它的外观几乎完全由「反射了什么环境」决定
- 物体看起来像悬浮在虚空中，与背景割裂

传统做法是加一个常数环境项 `ambient = 0.03 * albedo`。这能避免全黑，但它是各向同性的假常数：朝天的面和朝地的面得到完全一样的环境光，金属也依然照不亮。

### IBL 的核心想法

IBL 把「环境」当成一个包裹场景的巨大光源，用一张图像记录每个方向上射来的辐亮度（radiance）。这张图通常是 HDR 全景图（equirectangular，经纬映射），像素值是超过 1.0 的真实光强而非显示码值。

于是环境光照变成一个有明确物理定义的问题：给定表面法线 `n`、视线 `v`、材质参数，把这张图上所有方向的入射光按 BRDF 加权积分起来。

本项目使用的环境图是 `Content/Textures/HDR/citrus_orchard_road_puresky_4k.hdr`。

---

## 二、要计算的积分是什么

反射方程（渲染方程去掉自发光项）：

```text
L_o(p, ω_o) = ∫_Ω  f_r(p, ω_i, ω_o) · L_i(p, ω_i) · (n · ω_i)  dω_i
```

各项含义：

| 符号 | 含义 |
|------|------|
| `L_o` | 从表面点 `p` 沿观察方向 `ω_o` 射出的辐亮度，也就是最终像素颜色 |
| `Ω` | 法线 `n` 周围的上半球，所有可能的入射方向 |
| `L_i(ω_i)` | 从方向 `ω_i` 射来的辐亮度，IBL 中由环境贴图提供 |
| `f_r` | BRDF，描述材质如何把入射方向的光散射到出射方向 |
| `n · ω_i` | 兰伯特余弦项，入射角越斜，单位面积接收到的能量越少 |

直接光只需在几个离散方向上求值，而 IBL 需要对整个半球积分。逐像素做半球积分在实时渲染中不可行：一个 1080p 画面有两百万像素，每像素若采样上千方向，每帧就是数十亿次纹理读取。

**IBL 的全部工程内容，就是把这个积分中与运行时视角无关的部分提前算好，存成贴图。**

### 拆成 diffuse 与 specular

Cook-Torrance BRDF 可写成漫反射项加镜面项：

```text
f_r = k_d · f_lambert + f_cook-torrance
```

积分是线性的，因此可以分开处理：

```text
L_o = ∫ k_d · (albedo/π) · L_i · (n·ω_i) dω_i   ← diffuse IBL
    + ∫ f_specular · L_i · (n·ω_i) dω_i          ← specular IBL
```

这两项的难度差别很大，处理方式也完全不同。

---

## 三、Diffuse IBL：辐照度卷积

### 为什么 diffuse 可以精确预计算

Lambert BRDF 是常数 `albedo/π`，与入射方向和观察方向都无关，可以整体提到积分外：

```text
L_o,diffuse = (albedo/π) · ∫_Ω L_i(ω_i) · (n·ω_i) dω_i
                           └────────── E(n) ──────────┘
```

积分部分 `E(n)` 称为**辐照度（irradiance）**，单位是每单位面积的辐射通量。关键性质是：**它只依赖法线方向 `n`**，与视角、粗糙度、材质全都无关。

方向是二维量，因此 `E(n)` 恰好可以存进一张 cubemap：预先对每个方向做一次半球积分，运行时用法线直接查表。这张图叫 **irradiance map**。

由于结果只随法线缓慢变化（半球积分本身是强低通滤波），分辨率可以极低。本项目用 `32×32×6`，业界常见也就 32 或 64。

### 积分怎么离散化

在切线空间用球坐标参数化半球，立体角微元为 `dω = sinθ dθ dφ`，代入后：

```text
E(n) = ∫₀^{2π} ∫₀^{π/2} L_i(θ, φ) · cosθ · sinθ  dθ dφ
```

被积函数里出现两个三角函数，各有各的来源，容易混淆：

- `cosθ` 来自兰伯特余弦项 `n·ω_i`
- `sinθ` 来自球坐标下立体角微元的雅可比行列式，与光照无关，是坐标变换的产物

对常数环境 `L_i = L₀`，这个积分有解析解 `E = π · L₀`。这个结论是后面校验实现正确性的基准。

---

## 四、Specular IBL：分割求和近似

### 为什么 specular 不能像 diffuse 那样处理

镜面项的 BRDF 依赖观察方向 `ω_o`、法线 `n`、粗糙度和 F0 四个量。即使只离散化观察方向和法线，就已经是四维查找表，存不下也算不动。

Epic 在 SIGGRAPH 2013 的《Real Shading in Unreal Engine 4》中提出了 **split-sum approximation（分割求和近似）**，这是目前实时 specular IBL 的事实标准，本项目也采用它：

```text
∫ f_specular · L_i · (n·ω_i) dω_i  ≈  ( ∫ L_i · D(ω_i) dω_i ) · ( ∫ f_specular · (n·ω_i) dω_i )
                                       └── 预滤波环境图 ──┘    └────── BRDF LUT ──────┘
```

即把一个积分近似拆成两个可独立预计算的积分的乘积。这在数学上并不严格成立（乘积的积分不等于积分的乘积），但在实践中误差可以接受，代价是掠射角与高粗糙度下略有偏差。

### 第一项：预滤波环境贴图

`∫ L_i · D dω` 表示按 GGX 法线分布函数对环境图做加权模糊。粗糙度越高，GGX 波瓣越宽，模糊范围越大。

实现方式是把不同粗糙度的结果存进同一张 cubemap 的不同 mip 层级：mip 0 对应 roughness 0（清晰镜面），最高 mip 对应 roughness 1（完全漫散）。运行时用 `roughness` 计算 mip 层级，配合硬件的三线性过滤在层级间插值。

这里做了一个额外近似：真正的积分还依赖观察方向 `ω_o`，标准做法强制假设 `n = ω_o = ω_i`，即总是从法线方向观察。这会丢失掠射角下的拉长高光（stretched highlight）。

### 第二项：BRDF LUT

第二个积分与环境图无关，只依赖 `(n·ω_o)` 和 `roughness` 两个标量，恰好可存成一张二维查找表。

把 Fresnel-Schlick 展开后，F0 可以从积分中提取出来：

```text
∫ f_specular · (n·ω_i) dω_i  =  F0 · A + B
```

其中 A、B 是与 F0 无关的两个标量，分别存进 LUT 的 R、G 通道。这意味着**同一张 BRDF LUT 对所有材质通用**，与场景、环境图都无关，理论上可以离线烘焙一次永久复用。

LUT 通过 GGX 重要性采样（importance sampling）配合 Hammersley 低差异序列做蒙特卡洛积分得到。重要性采样的意义是：不均匀地撒采样点，而是按 GGX 分布本身的概率密度撒点，让样本集中在贡献大的区域，从而用少量样本得到低方差结果。

---

## 五、本项目的实现

所有 IBL 资源在 **CPU 上于启动期一次性生成**，入口是 `FRenderResourceManager::EnsureEnvironmentResources()`。这与常见的 GPU 预计算方案不同，取舍见「六、为什么选 CPU 预计算」。

### 5.1 生成链路总览

```text
citrus_orchard_road_puresky_4k.hdr
  └─ stbi_loadf 载入为 float RGBA（FHDRImage）
       ├─ GenerateEnvironmentCubePixels  → 环境 cubemap  128×128×6
       ├─ GenerateIrradianceCubePixels   → irradiance cubemap 32×32×6
       └─ GenerateBRDFLUTPixels          → BRDF LUT 2D 128×128
            └─ RHIDevice::CreateTexture(RGBA32_Float, generateMips=true)
                 └─ Environment BindGroup (group 4, binding 9/10/11)
                      └─ model.frag / deferred_lighting.frag / sky.frag
```

关键参数集中在 `RenderResourceManager.cpp` 顶部：

```38:43:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
constexpr uint32_t EnvironmentCubeSize = 128;
constexpr uint32_t IrradianceCubeSize = 32;
constexpr uint32_t BRDFLUTSize = 128;
constexpr uint32_t IrradiancePhiSamples = 48;
constexpr uint32_t IrradianceThetaSamples = 16;
constexpr uint32_t BRDFSampleCount = 128;
```

### 5.2 Equirectangular 到 Cubemap

HDR 全景图是经纬映射的二维图，需要先转成 cubemap，因为 GPU 用方向向量采样 cubemap 是硬件原生操作，且没有极点处的采样密度畸变。

方向到 UV 的映射（`SampleEquirectangularHDR`）：

```91:97:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
[[nodiscard]] Vector3 SampleEquirectangularHDR(const FHDRImage& image, const Vector3& direction)
{
    const Vector3 dir = direction.Normalize();
    const float u = std::atan2(dir.Z, dir.X) / Math::TWO_PI + 0.5f;
    const float v = 0.5f - std::asin(Math::Clamp(dir.Y, -1.0f, 1.0f)) / Math::PI;
    return SampleHDRBilinear(image, u, v);
}
```

- `atan2(z, x)` 给出水平方位角，除以 `2π` 归一化到 `[-0.5, 0.5]`，加 `0.5` 平移到 `[0, 1]`
- `asin(y)` 给出仰角，`v = 0` 对应正上方，符合图像原点在左上的约定
- U 方向环绕（`u - floor(u)`），V 方向钳制，因为经度是周期的而纬度不是

反方向的映射（cube 面上的像素对应哪个世界方向）在 `GetCubeFaceDirection` 中按 OpenGL cubemap 的六面朝向约定硬编码，包含每个面各自的 Y/X 翻转规则：

```99:112:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
[[nodiscard]] Vector3 GetCubeFaceDirection(uint32_t face, float u, float v)
{
    const float x = 2.0f * u - 1.0f;
    const float y = 2.0f * v - 1.0f;
    switch (face)
    {
    case 0: return Vector3(1.0f, -y, -x).Normalize();   // +X
    case 1: return Vector3(-1.0f, -y, x).Normalize();   // -X
    case 2: return Vector3(x, 1.0f, y).Normalize();     // +Y
    case 3: return Vector3(x, -1.0f, -y).Normalize();   // -Y
    case 4: return Vector3(x, -y, 1.0f).Normalize();    // +Z
    default: return Vector3(-x, -y, -1.0f).Normalize(); // -Z
    }
}
```

这张表是 cubemap 实现中最容易出错的地方。写错某一面的符号，表现是天空盒某个方向镜像或上下颠倒，而其余方向正常。排查时的有效手段是给六个面各填一个纯色再观察朝向。

### 5.3 Irradiance Cubemap 的生成

对每个像素求半球积分，采用**均匀网格黎曼和**而非蒙特卡洛，共 `48 × 16 = 768` 个方向样本：

```166:187:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
                Vector3 irradiance = Vector3::Zero;
                float weight = 0.0f;
                for (uint32_t phiIndex = 0; phiIndex < IrradiancePhiSamples; ++phiIndex)
                {
                    const float phi = (static_cast<float>(phiIndex) + 0.5f) / static_cast<float>(IrradiancePhiSamples) * Math::TWO_PI;
                    for (uint32_t thetaIndex = 0; thetaIndex < IrradianceThetaSamples; ++thetaIndex)
                    {
                        const float theta = (static_cast<float>(thetaIndex) + 0.5f) / static_cast<float>(IrradianceThetaSamples) * Math::HALF_PI;
                        const float sinTheta = std::sin(theta);
                        const float cosTheta = std::cos(theta);
                        const Vector3 sampleDir = (tangent * (std::cos(phi) * sinTheta) +
                                                   bitangent * (std::sin(phi) * sinTheta) +
                                                   n * cosTheta).Normalize();
                        const float sampleWeight = cosTheta * sinTheta;
                        irradiance += SampleEquirectangularHDR(image, sampleDir) * sampleWeight;
                        weight += sampleWeight;
                    }
                }
                if (weight > 0.0f)
                {
                    irradiance = irradiance * (Math::PI / weight);
                }
```

三个实现要点：

**采样点取格心。** `(index + 0.5) / count` 而非 `index / count`，是中点法则（midpoint rule）。落在区间中心比落在边界的积分精度更高，也避免 `θ = 0` 处 `sinθ = 0` 导致的整行样本权重为零。

**切线基的构造。** 积分在以 `n` 为极轴的局部球坐标系中进行，需要临时正交基。`BuildTangentBasis` 在 `n` 接近竖直时把参考向量从 `Up` 换成 `Right`，避免叉乘退化：

```143:148:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
void BuildTangentBasis(const Vector3& n, Vector3& outTangent, Vector3& outBitangent)
{
    const Vector3 up = std::abs(n.Y) < 0.999f ? Vector3::Up : Vector3::Right;
    outTangent = Vector3::Cross(up, n).Normalize();
    outBitangent = Vector3::Cross(n, outTangent).Normalize();
}
```

这个基是任意旋转的（绕 `n` 的方位不确定），但对全半球积分来说无所谓，因为积分对绕 `n` 的旋转不变。

**归一化方式。** 代码没有显式乘 `Δθ · Δφ`，而是除以权重和 `Σ cosθ sinθ` 再乘 `π`。这两者是等价的：因为 `Σ cosθ sinθ · ΔθΔφ ≈ ∫cosθ sinθ dθdφ = π`，所以 `Σ cosθ sinθ ≈ π / (ΔθΔφ)`，于是 `π / Σ ≈ ΔθΔφ`。好处是即使样本数改变或分布不均，归一化仍自动成立。

代入常数环境 `L₀` 验证：结果为 `L₀ · Σw · π / Σw = π · L₀`。这与解析解 `E = π L₀` 一致，说明**本项目的 irradiance map 存的是真正的辐照度 E**。这个结论在「七、当前偏差」中很关键。

### 5.4 BRDF LUT 的生成

`IntegrateBRDF(nDotV, roughness)` 用 128 个 Hammersley 样本做 GGX 重要性采样：

```238:262:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
[[nodiscard]] Vector2 IntegrateBRDF(float nDotV, float roughness)
{
    const Vector3 v(std::sqrt(std::max(1.0f - nDotV * nDotV, 0.0f)), 0.0f, nDotV);
    const Vector3 n(0.0f, 0.0f, 1.0f);
    float a = 0.0f;
    float b = 0.0f;
    for (uint32_t i = 0; i < BRDFSampleCount; ++i)
    {
        const Vector2 xi = Hammersley(i, BRDFSampleCount);
        const Vector3 h = ImportanceSampleGGX(xi, n, roughness);
        const Vector3 l = (h * (2.0f * Vector3::Dot(v, h)) - v).Normalize();
        const float nDotL = std::max(l.Z, 0.0f);
        const float nDotH = std::max(h.Z, 0.0f);
        const float vDotH = std::max(Vector3::Dot(v, h), 0.0f);
        if (nDotL > 0.0f)
        {
            const float g = GeometrySmithIBL(nDotV, nDotL, roughness);
            const float gVis = (g * vDotH) / std::max(nDotH * nDotV, 0.0001f);
            const float fc = std::pow(1.0f - vDotH, 5.0f);
            a += (1.0f - fc) * gVis;
            b += fc * gVis;
        }
    }
    return Vector2(a / static_cast<float>(BRDFSampleCount), b / static_cast<float>(BRDFSampleCount));
}
```

逐条说明：

**积分在固定局部坐标系中进行。** 法线固定为 `(0,0,1)`，视线由 `nDotV` 反解出来放在 XZ 平面上。因为 BRDF 各向同性，绕法线旋转不影响结果，所以不需要遍历真实法线方向，LUT 才能只有二维。

**Hammersley 序列。** `RadicalInverseVdc` 用位反转生成 Van der Corput 序列，配合 `i/N` 组成二维低差异序列。相比伪随机数，低差异序列的样本分布更均匀，蒙特卡洛收敛速度从 `O(1/√N)` 改善到接近 `O(1/N)`，这是 128 个样本就够用的原因。位反转的实现是经典的分治位交换：

```197:205:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
float RadicalInverseVdc(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
}
```

**`ImportanceSampleGGX` 采的是半程向量 H 而非入射方向 L。** GGX 的法线分布 D 描述的是微表面法线（即 H）的分布，所以重要性采样必须按 H 的分布采样，再通过 `L = 2(V·H)H - V` 反射得到入射方向。这一步顺序颠倒会导致 LUT 完全错误。

**`gVis` 中的除法是重要性采样的 PDF 抵消。** 按 GGX 分布采样时 PDF 含有 `D·(n·h)/(4·(v·h))` 因子，与被积函数中的 `D` 约掉后只剩几何项与投影因子，因此最终求和式里看不到 `D`。这是重要性采样最漂亮的地方：选对 PDF 后，最难算的项自动消失。

**IBL 专用的 k。** 注意 `GeometrySchlickGGX` 这里用 `k = a²/2`：

```226:231:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
float GeometrySchlickGGX(float nDotV, float roughness)
{
    const float a = roughness;
    const float k = (a * a) / 2.0f;
    return nDotV / std::max(nDotV * (1.0f - k) + k, 0.0001f);
}
```

而直接光照的 shader 里用的是 `k = (r+1)²/8`（见 `model.frag:62-67`）。这不是笔误：Disney/UE4 对直接光和 IBL 使用了不同的 k 重映射，因为两者的入射光分布假设不同。同名函数在两处取值不同，是阅读 PBR 代码时的经典困惑点。

**LUT 的 V 轴是翻转的。** `roughness = 1.0 - (y+0.5)/size`，因为纹理原点在左上而希望 roughness 从下往上增长。采样时 `texture(u_BRDFLUT, vec2(nDotV, roughness))` 与之配套，两处必须同时改否则 LUT 上下颠倒。

### 5.5 GPU 资源创建

三张贴图统一走 `RHITextureDesc`，格式全部是 `RGBA32_Float`：

```729:737:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
        RHITextureDesc desc;
        desc.dimension = RHITextureDimension::TextureCube;
        desc.width = size;
        desc.height = size;
        desc.format = RHIFormat::RGBA32_Float;
        desc.initialData = pixels.data();
        desc.generateMips = true;
        desc.srgb = false;
        desc.debugName = debugName;
```

两个关键设置：

- `format = RGBA32_Float`：环境光是 HDR 数据，天空亮度可以远超 1.0。用 `RGBA8` 会把所有超过 1 的值截断，高光完全消失。代价是显存占用，`128×128×6×16 字节 = 1.5 MB`，当前规模可接受；生产环境通常用 `RGBA16F` 或 `RG11B10F` 减半。
- `srgb = false`：HDR 文件本身就是线性数据，绝不能走 sRGB 解码。BRDF LUT 存的是纯数值更不能。这与 [`PBR 材质与颜色空间基础`](./PBR材质与颜色空间基础.md) 中「数据贴图保持 Linear」的约定一致。

采样器 `Environment_Cubemap_Sampler` 用 `LinearMipmapLinear + ClampToEdge + 8x 各向异性`，三线性过滤是 specular 按 roughness 在 mip 间插值的前提。

### 5.6 绑定到 Shader

三个 IBL 资源占据 group 4（`RendererBindGroups::Environment`）的 binding 9/10/11：

```340:342:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
    desc.entries.push_back({RendererBindings::IrradianceMap, RHIBindingType::TextureCube, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::PrefilterMap, RHIBindingType::TextureCube, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::BRDFLUT, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
```

Shader 侧通过公共宏声明对应槽位，OpenGL 下 group 参数被丢弃只保留 binding，Vulkan 下展开为 `set = group`：

```36:38:Content/Shaders/OpenGL/model.frag
TE_RESOURCE_BINDING(4, 9) uniform samplerCube u_IrradianceMap;
TE_RESOURCE_BINDING(4, 10) uniform samplerCube u_PrefilterMap;
TE_RESOURCE_BINDING(4, 11) uniform sampler2D u_BRDFLUT;
```

### 5.7 Shader 中的消费

Forward 路径的 IBL 计算集中在 `model.frag` 的 8 行：

```121:132:Content/Shaders/OpenGL/model.frag
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    float nDotV = max(dot(n, v), 0.0);
    vec3 f = FresnelSchlickRoughness(nDotV, f0, roughness);
    vec3 kS = f;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    vec3 irradiance = texture(u_IrradianceMap, n).rgb;
    vec3 diffuseIBL = irradiance * baseColor;
    vec3 reflection = reflect(-v, n);
    vec3 prefilteredColor = textureLod(u_PrefilterMap, reflection, roughness * 4.0).rgb;
    vec2 brdf = texture(u_BRDFLUT, vec2(nDotV, roughness)).rg;
    vec3 specularIBL = prefilteredColor * (f * brdf.x + brdf.y);
    vec3 color = (kD * diffuseIBL + specularIBL) * ao;
```

几个必须理解的细节：

**`f0 = mix(0.04, baseColor, metallic)`。** 电介质的垂直入射反射率约 4%，且几乎无色；金属的反射率高且带颜色，其「颜色」实际来自 F0 而非漫反射。这一行同时处理了两类材质，`metallic` 作为插值因子。

**为什么用 `FresnelSchlickRoughness` 而非普通 Fresnel。** 标准 Fresnel-Schlick 是针对单一入射方向的。IBL 是整个半球的积分结果，粗糙表面的边缘不应该像光滑表面那样出现强烈的掠射高光。这个变体用 `max(1-roughness, f0)` 替代上限 1.0，让粗糙材质的边缘 Fresnel 增强被抑制。

**`kD = (1 - kS)(1 - metallic)`。** 前一项是能量守恒：被镜面反射走的能量不能再参与漫反射。后一项是金属没有次表面散射，漫反射为零。

**`textureLod(..., roughness * 4.0)`。** 这里的 `4.0` 是硬编码的粗糙度到 mip 层级映射，对应 `128×128` cubemap 的 mip 0..4（`128 → 64 → 32 → 16 → 8`）。这个常数与 `EnvironmentCubeSize` 隐式耦合：改了 cubemap 尺寸而忘记改这里，高粗糙度材质的反射会突然变清晰或采到无效 mip。

**`f * brdf.x + brdf.y` 就是 split-sum 的第二项。** 对应「四、第二项」中的 `F0 · A + B`，A、B 从 LUT 的 R、G 通道读出。

Deferred 路径在 `deferred_lighting.frag` 中有完全相同的一段逻辑，两边必须手工保持一致。

**天空背景**是同一套资源的另一种用法（`sky.frag`）：全屏三角形上，用 inverse view-projection 把屏幕 UV 反投影到远平面得到世界方向，直接采样 cubemap。它不需要深度缓冲，也不需要几何：

```21:29:Content/Shaders/OpenGL/sky.frag
void main()
{
    vec2 ndc = vScreenUV * 2.0 - 1.0;
    vec4 world = u_InvViewProjection * vec4(ndc, 1.0, 1.0);
    world.xyz /= max(world.w, 0.0001);
    vec3 dir = normalize(world.xyz - u_CameraPosition_Pad.xyz);
    vec3 sky = textureLod(u_PrefilterMap, dir, 0.0).rgb;
    fragColor = vec4(TonemapReinhard(sky), 1.0);
}
```

---

## 六、为什么选 CPU 预计算

标准做法（LearnOpenGL、UE、Filament）是在 GPU 上预计算：把 cubemap 六个面绑成 render target，用一个 fragment shader 完成卷积。本项目选择了 CPU。

选择理由：

- **时序上更简单。** GPU 预计算需要离屏 RenderTarget、cubemap 作为渲染目标、逐 mip 渲染等能力。这些在本项目的 RHI 抽象里当时尚未完备（Vulkan 后端至今仍不支持离屏 RT 与 TextureCube），CPU 路径可以完全绕开后端差异。
- **可调试性。** CPU 代码可以断点、可以单步、可以直接打印中间值。GPU 卷积出错时只能靠输出图像反推。
- **规模可控。** 当前只需 `32×32×6 = 6144` 个像素各 768 次采样，加上 `128×128 = 16384` 个 LUT 像素各 128 次采样，量级在启动期一次性开销可接受的范围内。

放弃的代价：

- 启动期有可感知的卡顿，且是单线程串行的
- 不支持运行时切换环境图或动态天空
- 分辨率无法提高，否则启动时间线性增长

后续若要支持动态天光或更高分辨率，正确方向是迁移到 GPU compute，或改用球谐（SH）表示 irradiance：9 个三阶 SH 系数就能几乎无损地表示辐照度，把 cubemap 采样替换成 9 次乘加，且卷积本身也可以在 SH 域高效完成。

---

## 七、当前偏差与已知限制

本节记录本项目实现与标准做法的差距。这部分对学习尤其重要，因为它们都是「看起来能跑但物理上不对」的情况。

### 7.1 Diffuse IBL 亮了 π 倍（未修复）

如 5.3 所述，本项目的 irradiance map 存的是真正的辐照度 `E = π·L₀`。而 Lambert 的正确出射为：

```text
L_o,diffuse = (albedo / π) · E
```

Shader 中写的是 `diffuseIBL = irradiance * baseColor`，缺少 `1/π`，因此 diffuse 环境光偏亮约 3.14 倍。

需要特别注意与 LearnOpenGL 的差异：LearnOpenGL 的卷积代码归一化为 `PI * irradiance / nrSamples`，存进去的其实是 `E/π` 而非 `E`，所以它的 shader 里直接乘 albedo 是正确的。本项目的归一化用了 `π / Σweight`，数学上多乘了一个 π，但 shader 照搬了 LearnOpenGL 的写法，于是量纲对不上。

实际观感是：物体环境漫反射整体偏亮偏灰，diffuse 相对 specular 过强，金属与非金属的对比被削弱。因为缺少 tonemapping 和曝光控制（见 7.3），这个偏差不会表现为明显的过曝白块，反而容易被误认为「风格问题」。

修复方式二选一，不可同时做：在 `GenerateIrradianceCubePixels` 中把 `Math::PI / weight` 改为 `1.0f / weight`，或在两个 shader 中把 `diffuseIBL` 改为 `irradiance * baseColor / PI`。前者省一次除法且与 LearnOpenGL 语义一致，后者保持 irradiance map 的物理量纲正确。

### 7.2 Specular Prefilter 是占位实现

`PrefilterMap` 目前直接复用环境 cubemap，没有做任何 GGX 卷积：

```769:770:Source/Runtime/Renderer/Private/RenderResourceManager.cpp
    // 当前阶段先使用环境 cubemap 的 mip 链作为初版 specular prefilter，后续可替换为 GGX 预滤波 cubemap。
    m_EnvironmentIBLResources.PrefilterMap = m_EnvironmentIBLResources.EnvironmentMap;
```

mip 链由 `glGenerateMipmap` 生成，本质是 box filter 逐级下采样，与 GGX 波瓣形状无关。差异表现在：

- 高粗糙度反射不够柔和，仍保留环境图的高频结构
- 粗糙度与模糊程度的对应关系不符合物理，中等粗糙度尤其明显
- `roughness * 4.0` 的线性映射也是拍脑袋的，正确做法应基于 GGX 波瓣立体角推导

`Docs/architecture/PBR渲染前置规划.md` 已把正式 GGX prefilter 列为待实现项。

### 7.3 天空做了 tonemap，几何体没有（未修复）

`sky.frag` 与 `deferred_lighting.frag` 的 `GetSkyColor` 对天空背景做了 Reinhard tonemap，但物体的最终颜色直接输出线性值，不经过任何色调映射：

- 天空经过 `c/(c+1)` 压缩，永远不会超过 1.0
- 物体的 `fragColor` 直接输出，超过 1.0 的部分被硬截断

结果是天空与物体处在两套不同的亮度响应曲线上，物体高光容易死白，且两者的相对亮度关系不可信。真正的做法是整帧渲染到 HDR RenderTarget，在统一的后处理 pass 中做曝光和 tonemapping，天空与几何体走同一条曲线。

在补上这个后处理 pass 之前，调整 IBL 强度或对比物体与背景亮度都得不到可靠结论。

### 7.4 其他限制

- **Cubemap 分辨率偏低。** `128×128` 用作 irradiance 的输入足够，但直接当天空背景显示时，在 1080p 全屏下明显模糊。天空理应使用原始 HDR 或更高分辨率的独立 cubemap。
- **单一环境，无反射探针。** 整个场景共用一张环境图，无法表达室内外差异或局部反射。下一步通常是引入 reflection probe 与视差校正。
- **无水平遮挡。** irradiance 积分假设整个半球都可见，不考虑物体自身或场景的遮挡。这是 SSAO / 烘焙 AO 要解决的问题，当前只有材质 AO 贴图做粗略补偿。
- **Vulkan 后端不支持。** 阶段 B 的 `VulkanTexture` 只支持单 mip Texture2D，`bSupportsFullSceneRendering = false`，因此 Vulkan 路径完全不走 IBL。

---

## 八、动手验证建议

想确认自己真的理解了，可以做这几件事：

1. **验证 irradiance 的量纲。** 临时把 `SampleEquirectangularHDR` 的返回值改成常数 `(1,1,1)`，打印生成结果。若得到 `≈3.1416` 就证实了 7.1 的分析；改成 `1.0f / weight` 归一化后应得到 `≈1.0`。
2. **可视化 BRDF LUT。** 把 `GenerateBRDFLUTPixels` 的结果直接当作全屏纹理输出。正确的 LUT 左下角接近 `(1, 0)`，右上角随粗糙度升高整体变暗，形状应与 UE4 论文中的图一致。
3. **只看 IBL。** 在 `model.frag` 中注释掉方向光和点光的循环，只保留 `color = (kD * diffuseIBL + specularIBL) * ao`。此时把 `roughness` 强制为 0 应得到近似镜面的环境反射，强制为 1 应得到接近纯 irradiance 的柔和结果。
4. **验证 cube 面朝向。** 让 `GenerateEnvironmentCubePixels` 给六个面各填不同纯色，观察天空盒，确认 +Y 在头顶、+Z 在预期的前方。

---

## 九、延伸阅读与代码索引

主要代码位置：

| 内容 | 位置 |
|------|------|
| IBL 资源生成全流程 | `Source/Runtime/Renderer/Private/RenderResourceManager.cpp:39-282`、`679-777` |
| Environment BindGroupLayout | 同上 `:333-345` |
| 资源查询接口 | 同上 `:1043-1052`、`Renderer/Public/RenderResourceManager.h` |
| Environment BindGroup 构建 | `Source/Runtime/Renderer/Private/RendererTextureBindings.cpp` |
| Forward IBL 着色 | `Content/Shaders/OpenGL/model.frag:119-132` |
| Deferred IBL 着色 | `Content/Shaders/OpenGL/deferred_lighting.frag` |
| 天空背景 | `Content/Shaders/OpenGL/sky.frag` |

相关文档：

- [PBR 材质与颜色空间基础](./PBR材质与颜色空间基础.md)：Linear/sRGB 约定，理解 `srgb = false` 的前提
- [引擎学习路径与知识脉络](./引擎学习路径与知识脉络.md)：IBL 在整个学习路径中的位置
- [渲染管线](../guides/渲染管线.md)：IBL 资源在 Forward / Deferred 帧流程中的绑定时机
- [PBR 渲染前置规划](../architecture/PBR渲染前置规划.md)：GGX prefilter、Tonemapping 等待实现项
- [Shader 资源绑定清单](../reference/Shader资源绑定清单.md)：group/binding 的完整对照表
- [图形与开发踩坑记录](../reference/图形与开发踩坑记录.md)：7.1 与 7.3 的问题条目

外部参考：

- Karis, B. *Real Shading in Unreal Engine 4*, SIGGRAPH 2013 —— split-sum approximation 的原始出处
- LearnOpenGL, *Diffuse irradiance* / *Specular IBL* —— 本项目实现的主要参考，注意其 irradiance 归一化与本项目不同
- Filament 文档 *Image Based Lights* 章节 —— 生产级实现的细节，含 SH 与预滤波的完整推导
