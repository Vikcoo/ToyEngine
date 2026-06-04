# 架构优化复盘：多 API 后端兼容性改造第一阶段

> 日期：2026-04-16

## 背景：为什么要改

ToyEngine 在此前的开发中只有 OpenGL 后端，RHI 抽象层虽然已经独立于具体后端，但上层代码中存在大量隐式依赖 OpenGL 行为的逻辑。经过系统分析，识别出 10 个维度的兼容性缺陷，这些缺陷会在引入 Vulkan 或 D3D12 后端时导致不可忽略的返工。

核心问题包括：

1. **纹理坐标原点**：`AssetImporter` 使用 `aiProcess_FlipUVs`（适配 OpenGL 左下角原点），但 `stbi_load` 默认从上到下加载像素。两者的"相互补偿"形成了脆弱的隐式约定。
2. **NDC 深度范围**：`CameraComponent` 硬编码使用 `PerspectiveGL()`（[-1,1] 深度），而 Vulkan/D3D12 原生为 [0,1]。
3. **NDC Y 轴方向**：Vulkan 的 NDC Y 轴向下，与 OpenGL/D3D12 相反，未做任何预留。
4. **Uniform 接口**：`RHICommandBuffer` 的 `SetUniformMatrix4` 等方法是 name-based（OpenGL 专属），Vulkan/D3D12 使用 Descriptor Set / Root Signature，完全不同的绑定模型。
5. **Platform 层与 OpenGL 绑死**：`GLFWWindow` 构造时直接创建 OpenGL Context 并加载 glad，Vulkan/D3D12 无法复用。
6. **Shader 路径硬编码**：`RenderResourceManager` 硬编码 `Content/Shaders/OpenGL/`。
7. **缺少 RenderTarget 抽象**：无法支持离屏渲染、MRT、后处理。
8. **sRGB 处理未落地**：`RHITextureDesc.srgb` 字段被忽略。

## 这次修改的核心思路

**引擎内部统一使用一套"规范约定"，各后端在 RHI 实现层做差异适配，上层代码不感知后端差异。**

这是 UE5 的标准做法：引擎层定义自己的坐标系、深度范围、纹理原点约定，由 RHI 后端在边界处做转换。

### 引擎规范约定

| 维度      | 引擎约定        | 说明                    |
| ------- | ----------- | --------------------- |
| 坐标系     | 右手 Y-up   | 与 GLM/OpenGL 一致     |
| NDC 深度  | [0, 1]      | 采用现代 API 标准，精度更优  |
| NDC Y 轴 | 向上          | 与 OpenGL/D3D12 一致   |
| 纹理原点    | 左上角         | 与图片文件存储一致，减少翻转   |
| 矩阵主序    | 列主序         | 与 GLM 一致，Shader 中适配 |
| 正面缠绕    | 逆时针         | 与当前一致               |

### 各后端适配策略

- **OpenGL**：纹理上传时翻转像素行序；当前以 OpenGL 4.5 Core 为目标，默认启用 `glClipControl` 适配 [0,1] 深度；若未来降级到更低版本，再回退矩阵重映射路径。
- **Vulkan**（预留）：投影矩阵 Y 分量取反（NDC Y 向下），其余天然对齐。
- **D3D12**（预留）：天然对齐，无需额外处理。

## 分阶段改动总结

### 阶段 0：后端特征描述基础设施

- `RHITypes.h` 新增 `ERHIBackendType` 枚举和 `RHIBackendTraits` 结构体，描述后端的原生 NDC 深度、Y 轴方向、纹理原点等特征。
- `RHIDevice` 新增纯虚方法 `GetBackendTraits()`。
- `OpenGLDevice` 初始化时填充 traits；当前 OpenGL 4.5 Core 路径直接启用 `glClipControl`，低版本回退路径仍保留在 `AdjustProjectionMatrix()`。

### 阶段 1：纹理原点统一

- `AssetImporter.cpp`：移除 `aiProcess_FlipUVs`，引擎约定纹理原点为左上角，不再在导入阶段翻转 UV。
- `OpenGLTexture.cpp`：在纹理上传前翻转像素行序（适配 OpenGL 的左下角原点），同时修复 `ConvertFormat` 对 `desc.srgb` 的忽略，正确使用 `GL_SRGB8` / `GL_SRGB8_ALPHA8`。

### 阶段 2：NDC 深度范围统一

- `CameraComponent::BuildViewInfo()`：从 `Matrix4::PerspectiveGL()`（[-1,1]）改为 `Matrix4::Perspective()`（[0,1]）。
- `RHIDevice` 新增虚方法 `AdjustProjectionMatrix()`，默认原样返回。
- `OpenGLDevice::AdjustProjectionMatrix()`：在 `glClipControl` 已启用时直接透传 [0,1] 深度投影矩阵；若未来回退到低版本 OpenGL，再乘深度重映射矩阵将 [0,1] 变换到 [-1,1]。Vulkan 后端未来在此处翻转 Y 轴。
- `SceneRenderer::SubmitDrawCommands()` 在提交渲染前调用 `AdjustProjectionMatrix`，每帧只做一次。

### 阶段 3：资源绑定模型（BindGroup）

- `RHITypes.h` 新增 `RHIBindingType`、`RHIBindGroupLayoutEntry`、`RHIBindGroupEntry`、`RHIBindGroupDesc` 等描述结构。
- 新增 `RHIBindGroup` 抽象接口。
- `RHIDevice` 新增 `CreateBindGroup()` 纯虚方法。
- `RHICommandBuffer` 新增 `SetBindGroup()` 纯虚方法，同时保留旧版 name-based uniform 接口用于过渡。
- OpenGL 后端实现 `OpenGLBindGroup`（缓存绑定信息）和 `OpenGLCommandBuffer::SetBindGroup()`（执行 `glBindBufferBase` / `glActiveTexture` + `glBindTexture`）。

### 阶段 4：Platform 层解耦

- `FWindowConfig` 新增 `EWindowGraphicsAPI` 枚举和 `graphicsAPI` 字段。
- `GLFWWindow`：根据 `graphicsAPI` 选择创建 OpenGL Context 还是 `GLFW_NO_API` 窗口。
- `IWindow`：`SwapBuffers` / `SetVSync` / `IsVSyncEnabled` 改为带默认空实现的虚方法，新增 `GetGraphicsAPI()` 查询方法。

### 阶段 5：着色器跨平台基础

- 第一阶段曾在 `RHIShaderDesc` 中预留后端原生路径，并由 Renderer 根据 `RHIBackendTraits::backendType` 选择着色器子目录。
- 该方案已由后续“阶段 7：逻辑 Shader 名称与后端资产解析”替代，保留在这里用于说明演进过程。

### 阶段 6：RenderTarget 抽象与 sRGB 格式

- `RHIFormat` 新增 `RGB8_sRGB`、`RGBA8_sRGB`、`D16_UNorm`、`D24_UNorm_S8_UInt`、`D32_Float` 格式。
- 新增 `RHIRenderTarget` 抽象接口和 `RHIRenderTargetDesc` / `RHIAttachmentDesc` 描述结构。
- `RHIRenderPassBeginInfo` 新增可选的 `renderTarget` 引用（nullptr 表示默认帧缓冲）。
- `RHIDevice` 新增 `CreateRenderTarget()` 纯虚方法。
- `OpenGLDevice::CreateRenderTarget()` 初始为占位实现（返回 nullptr + warn）；已在 `架构优化复盘-前向与延迟双渲染路径.md` 的 Stage 0 中补齐为真实 FBO 实现，落地 `OpenGLRenderTarget`。

### 阶段 7：逻辑 Shader 名称与后端资产解析（2026-06-04）

原有方案虽然能选择不同后端的 Shader 目录，但 `FRenderResourceManager` 与 `FDeferredRenderPath` 仍需查询 `backendType` 并拼接 `OpenGL/`、`Vulkan/`、`D3D12/` 路径。Renderer 因此知道了具体后端资产布局，新增后端或调整资产格式时仍需修改上层渲染代码。

本阶段调整为：

- `RHIShaderDesc` 只接收 `logicalName`、阶段、入口点和调试名，不再暴露 `filePath`、`spirvPath`、`spirvData` 等后端原生输入。
- Renderer 新增集中逻辑名表 `RendererShaderNames.h`，Pipeline 创建代码只请求 `StaticMesh/BasePassVS`、`Deferred/LightingPS` 等逻辑 Shader。
- `RHIOpenGL` 新增 `OpenGLShaderLibrary`，负责把逻辑名解析为 `Content/Shaders/OpenGL/` 下的 GLSL 文件。
- Shader 根目录编译定义从 `Renderer` 移到 `RHIOpenGL`，Renderer 不再知道后端 Shader 目录。

设计取舍：

- 当前映射表仍由 OpenGL 后端手工维护，尚未引入资产清单、离线编译或自动反射。
- 逻辑名由 Renderer 定义，原生资产解析由后端负责；未来 Vulkan/D3D12 可以对同一逻辑名分别解析 SPIR-V/DXIL，不需要修改 Renderer Pipeline 创建逻辑。
- 本阶段优先固定职责边界，不提前引入完整 UE ShaderMap、Permutation 和 Derived Data Cache 复杂度。

## 修改后带来的好处

1. **上层代码与后端无关**：CameraComponent、SceneRenderer、AssetImporter 等模块不再包含 OpenGL 特有逻辑，切换后端时这些代码无需修改。
2. **纹理行为可预测**：统一左上角原点约定，消除了 stb_image 和 Assimp FlipUVs 之间的隐式补偿。
3. **深度精度统一**：[0,1] 深度范围在所有后端行为一致，避免深度测试语义差异。
4. **资源绑定可扩展**：BindGroup 模型为 Vulkan DescriptorSet / D3D12 Descriptor Table 提供了对齐的抽象，name-based uniform 接口仅作为过渡保留。
5. **窗口可复用**：Vulkan/D3D12 后端可以使用 No-API 窗口，通过 `GetNativeHandle()` 自行创建 Surface/SwapChain。

## 仍然留下的限制

1. **旧版 Uniform 接口未移除**：主渲染路径已不再使用 `SetUniformMatrix4 / SetUniformInt / BindTexture2D`，但接口本身仍保留，用于少量示例路径和过渡期兼容。
2. ~~**OpenGL RenderTarget 未实现**~~（已在 `架构优化复盘-前向与延迟双渲染路径.md` 的 Stage 0 完成）：OpenGL 后端现已实装 `OpenGLRenderTarget`（FBO + MRT + 深度附件），`CreateRenderTarget` 返回真实对象，`BeginRenderPass` 依据 `renderTarget` 字段切换离屏 / 默认帧缓冲。
3. **着色器编译管线未建立**：当前已使用逻辑 Shader 名称隔离 Renderer 与后端资产路径，但 OpenGL 映射表仍为手写，GLSL → SPIR-V / DXIL、反射、缓存和自动资产清单尚未搭建。
4. **OpenGL 目标版本变更会牵动整条链路**：当前项目已切到 OpenGL 4.5 Core，`glClipControl` 可直接启用；如果后续再调整目标版本，必须同步检查 GLFW 上下文请求、glad 生成版本、GLSL `#version`、文档和兼容分支是否全部对齐。
5. **矩阵主序适配未实现**：D3D12 HLSL 默认行主序的适配（shader 中声明 `column_major` 或上传时转置）尚未处理。
6. **坐标系手性适配未实现**：D3D12 默认左手坐标系的正面缠绕差异尚未在后端层面处理。
