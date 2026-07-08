# PBR 材质与颜色空间基础

本文档记录 PBR 材质系统和颜色空间的基础概念，用于解释 `Docs/architecture/PBR渲染前置规划.md` 中的材质因子、纹理语义和 sRGB / Linear 规则。

本文只描述概念和项目约定，不代表完整工业级 PBR 渲染已经实现。当前项目资产侧已经具备最小 `FMaterial` 描述，渲染侧已经消费 BaseColor / Normal / Metallic / Roughness / AO / Emissive 贴图与材质因子，但 specular IBL 预滤波、曝光、Tonemapping 和 PBR 参数 DebugView 仍是后续工作。

## 材质 Factor 的含义

`BaseColorFactor`、`MetallicFactor`、`RoughnessFactor`、`EmissiveFactor` 中的 `Factor` 指材质参数的常量值或倍率，不是特殊的 shader 机制。

常见用法是：

```text
最终 BaseColor = BaseColorTexture 采样值 * BaseColorFactor
最终 Metallic  = MetallicTexture 采样值 * MetallicFactor
最终 Roughness = RoughnessTexture 采样值 * RoughnessFactor
最终 Emissive  = EmissiveTexture 采样值 * EmissiveFactor
```

当材质没有对应贴图时，Factor 就是稳定的默认材质输入。例如无贴图材质可以只依靠 `BaseColorFactor`、`MetallicFactor` 和 `RoughnessFactor` 渲染。

`BaseColorFactor` 通常也是 BaseColor 贴图的整体染色系数。没有 BaseColor 贴图时，它就是材质基础颜色；有贴图时，它会和贴图颜色相乘。

## 共享材质、材质实例与动态参数

多个模型 Section 如果引用同一个材质实例，并且几何、UV、光照、顶点属性和运行时覆盖参数也相同，那么它们会得到相同的材质输入和相近的渲染效果。

如果只修改共享材质资源本体，所有引用它的 Section 都会受到影响。因此更推荐区分：

- `Material`：描述 shader、贴图槽和默认因子。
- `MaterialInstance`：为某个对象或 Section 覆盖部分材质参数。
- `PerObject` / `PerDraw` 参数：每次绘制前更新的临时参数，例如受击高亮、溶解进度、自发光闪烁。

如果希望某个 Section 的参数随时间变化，通常由引擎运行时在 shader 外侧根据时间更新材质实例或 per-draw 参数，再在绘制前写入 UBO、Push Constant 或其它常量缓冲。

例如：

```text
EmissiveFactor = BaseEmissive * (0.5 + 0.5 * sin(Time))
```

shader 也可以接收 `Time`、`PulseSpeed`、`PulseStrength` 等显式参数并在内部计算动画效果，但这些规则应作为材质或效果设计的一部分，而不是隐藏地修改 PBR 参数。

## 同一 Section 内不同位置的材质差异

一次 draw call 绑定的普通材质 factor 通常对整个 Section 都相同。绘制前修改一个统一参数，只能让不同对象、不同 Section 或不同 draw call 之间产生差异，不能直接让同一个 Section 内每个位置拥有不同参数。

如果希望同一 Section 内不同位置表现不同，需要让 shader 能读取 per-pixel 或 per-vertex 的差异数据：

- 纹理贴图：通过 BaseColor、MetallicRoughness、AO、Mask 等贴图控制不同 UV 区域。
- 顶点属性：通过 Vertex Color、额外 UV、权重等传入局部参数。
- 空间函数：shader 根据 local position 或 world position 计算高度渐变、扫描线、区域变化。
- 数据纹理或 buffer：复杂情况下按空间坐标查询额外参数场。

PBR 材质中最常见、最资产友好的方式是贴图：BaseColor 贴图控制不同位置颜色，MetallicRoughness 贴图控制不同位置金属度和粗糙度，Normal 贴图控制不同位置法线细节。

## 纹理语义

纹理语义指一张纹理在材质中代表什么数据。渲染器不能只知道“这是一张图片”，还必须知道它是颜色、法线、金属度、粗糙度、AO 还是自发光。

常见语义包括：

- `BaseColorTexture`：物体基础颜色或反照率。
- `NormalTexture`：切线空间法线扰动。
- `MetallicTexture`：金属度。
- `RoughnessTexture`：粗糙度。
- `MetallicRoughnessTexture`：打包的金属度与粗糙度数据。
- `AOTexture`：环境遮蔽。
- `EmissiveTexture`：自发光颜色或强度。

语义会决定采样颜色空间、默认值、shader 读取通道、调试视图显示方式以及 GPU 纹理格式选择。Normal、Metallic、Roughness、AO 这类数据贴图绝不能被当成普通颜色贴图处理。

## sRGB 与 Linear 的区别

Linear 空间中的数值与物理光强或线性比例成正比：

```text
0.0 = 没有光或 0% 比例
0.5 = 一半光强或 50% 比例
1.0 = 全光强或 100% 比例
```

sRGB 是面向存储和显示的非线性编码。它会把更多 8-bit 编码精度分配给人眼更敏感的暗部，把较少精度分配给亮部。这样普通图片和颜色贴图在低位深下更不容易丢失暗部细节。

近似关系：

```text
Linear 0.01 -> sRGB 约 0.10
Linear 0.10 -> sRGB 约 0.35
Linear 0.50 -> sRGB 约 0.735
sRGB  0.50 -> Linear 约 0.214
```

因此 sRGB 适合存储和显示颜色，但不适合直接做光照计算。

## 为什么 PBR 计算要在线性空间完成

光照、颜色混合和 PBR BRDF 计算处理的是光能量、辐射亮度、反射比例等线性量。光源强度相加、颜色相乘、反射比例缩放和 BRDF 积分都默认输入具有线性意义。

如果直接在 sRGB 编码值上计算，相当于把非线性的显示码值当成真实光强，会造成系统性错误：

- 多光源叠加过亮或过暗。
- 颜色插值偏暗。
- 漫反射颜色发灰。
- 高光强度和范围不符合预期。
- IBL 预滤波和 mipmap 平均结果不稳定。
- 数据贴图若误走 sRGB 解码，会破坏 Roughness、Metallic、AO、Normal 等数值。

标准流程是：

```text
BaseColor / Emissive 颜色贴图：sRGB 存储
采样或加载时：sRGB -> Linear
shader 内部：Linear 空间做光照、PBR、混合
输出到屏幕前：Linear -> sRGB
```

数据贴图不走 sRGB 解码：

```text
Normal / Metallic / Roughness / AO：保持 Linear 数值
```

## 颜色贴图存的是颜色还是光强

颜色贴图当然存的是颜色外观，但文件中的颜色通常是 sRGB 编码值。进入 shader 做物理计算时，需要把它还原为具有线性意义的 RGB 数值。

对 `BaseColor` 来说，这个线性 RGB 值不是光源发出的光强，而是材质对入射光的反射比例或反照率。例如 `BaseColor = (0.8, 0.1, 0.1)` 可以理解为材质强烈反射红色通道，较少反射绿色和蓝色通道。

对 `Emissive` 来说，线性 RGB 值更接近自发光颜色或强度贡献。

对最终渲染结果来说，shader 输出的线性 RGB 通常表示当前像素的线性光照结果，最后再编码为 sRGB 交给显示设备。

## 其它颜色空间

ToyEngine 当前阶段优先固定 sRGB 与 Linear 的基础规则。更完整的渲染管线中还可能遇到其它颜色空间：

- `Rec.709`：传统 HDTV 视频常见色彩标准。
- `Display P3`：现代显示设备常见广色域。
- `Rec.2020`：UHD / HDR 视频常见广色域。
- `ACEScg`：影视和高质量渲染管线常用线性广色域工作空间。
- `ACES2065-1`：ACES 交换和归档空间。
- `Adobe RGB`：摄影和印刷流程常见色彩空间。
- `XYZ`：CIE 标准颜色空间，常作为颜色转换中间空间。
- `Lab`：更接近感知均匀性的颜色空间，常用于色差计算和图像处理。

这些内容可以等 HDR、曝光、Tonemapping、IBL 和广色域输出需求出现后再扩展。当前阶段最重要的项目约定是：

```text
颜色贴图按 sRGB 读取并转换到 Linear
数据贴图保持 Linear
shader 内部统一 Linear 计算
最终显示前再转换到 sRGB
```
