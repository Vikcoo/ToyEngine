# 架构优化复盘：Vulkan 后端阶段 B

## 阶段目标

阶段 A 只证明了 Instance、Device、Swapchain 与帧同步闭环，无法验证 Renderer 的资源契约是否真的能落到显式 API。阶段 B 的目标不是立即迁移完整 PBR 场景，而是建立一个最小但真实的静态网格垂直切片：从 GPU 内存、上传、Shader、Descriptor、Pipeline，一直走到 indexed draw 和深度测试。

## 原有结构的问题

- Vulkan 工厂只能创建帧级对象，`CreateBuffer/CreateTexture/CreatePipeline` 等接口全部拒绝调用。
- `bSupportsSceneRendering` 只有“清屏”和“完整场景”两个状态。若直接开启，会立即触发 TextureCube、mip、IBL、MRT 等阶段 C 能力；保持关闭又无法让 Renderer 验证阶段 B。
- 阶段 A 没有深度附件，无法校验项目统一的 Reversed-Z、正面缠绕与投影 Y 适配。
- SPIR-V 已能离线生成，但尚无运行时 ShaderModule 路径，也没有 Descriptor 与 PipelineLayout 落地。

## 核心方案与取舍

### 用能力分层代替后端类型分支

`RHIBackendTraits` 新增 `bSupportsFullSceneRendering`：

- `bSupportsSceneRendering` 表示后端可以执行一个真实 SceneRenderer 绘制路径。
- `bSupportsFullSceneRendering` 表示后端已能运行应用提供的完整 Forward/Deferred/PBR 场景。

Vulkan 阶段 B 为 `true/false`，Engine 延后 Sandbox 场景回调，`FSceneRenderer` 改走内部静态网格验证路径。OpenGL 默认为 `true/true`，行为不变。这样阶段能力由契约表达，Engine 和 Renderer 不需要判断 `ERHIBackendType::Vulkan`。

### 正确性优先的资源实现

Buffer 与 Image 当前各自调用一次 `vkAllocateMemory`，CPU 可见 Buffer 持久映射，GPU-only 初始数据通过短生命周期 Staging Buffer 上传。方案实现简单、错误边界清晰，足以验证 RHI usage/memory 描述；代价是分配次数多、缺少子分配与碎片治理，因此不作为最终显存管理器。

上传使用专用一次性 CommandPool/CommandBuffer，并在提交后 `vkQueueWaitIdle`。它适合启动期小资源，不适合流式资产。后续应改为上传 ring、transfer batch 和 fence retirement，而不是把当前同步路径扩散到每帧资源更新。

### 独立验证路径而不是提前迁移完整 Renderer

`FStaticMeshValidationRenderPath` 使用内置 cube 与 4×4 非对称 sRGB 纹理，专门覆盖：

- GPU-only Vertex/Index Buffer 与 Staging Copy；
- Texture2D upload、ImageView 与 Sampler；
- set 0 dynamic UBO、set 1 combined image sampler；
- SPIR-V ShaderModule、PipelineLayout、Dynamic Rendering Pipeline；
- per-frame transient Uniform ring、indexed draw、D32 深度和 Reversed-Z。

没有复用 Sandbox PBR 模型，是因为该路径会同时要求 cubemap、mip、IBL、多个材质组与离屏目标，无法区分阶段 B 基础能力错误和阶段 C 功能缺失。验证路径属于 Renderer 内部阶段设施，不成为游戏层公共 API。

### 坐标与深度继续由 RHI 契约统一

Vulkan 继续在 `AdjustProjectionMatrix()` 翻转 Y，Viewport 使用正高度。投影翻转会改变屏幕空间 winding，因此 Vulkan Pipeline 将 RHI 的 CCW 映射为 `VK_FRONT_FACE_CLOCKWISE`，避免双重 Y 适配或错误剔除。深度使用 `D32_Float`、clear 0、compare `Greater`，与 OpenGL Reversed-Z 基线一致。

## 修改后的收益

- Vulkan 已从“能清屏”进入“能消费真实 Renderer 资源契约”的阶段。
- Descriptor set/group、dynamic offset、Pipeline attachment format 和资源 usage 不再只是为未来预留的抽象，已经由真实后端执行。
- 阶段能力门控允许后续逐步启用 TextureCube、MRT、完整材质和 PBR，而不需要一次性大爆炸式迁移。
- 非对称纹理、旋转网格和背面裁剪提供了比纯清屏更强的方向、颜色空间与深度诊断信号。

## 当前限制与后续方向

- 仅支持单 mip、单层、单采样 `Texture2D`；TextureCube、自动 mip 和 MSAA 待实现。
- 仅支持默认交换链目标；离屏 RenderTarget、MRT 和 GBuffer 属于后续阶段。
- DescriptorPool 为固定容量单池，尚无分页、回收批次或耗尽策略。
- 资源包装对象必须活到 GPU 使用结束；通用 deferred destruction / fence retirement 尚未实现。
- 一次性上传会等待 Graphics Queue idle；阶段 C 前应设计批量上传和延迟释放。
- 完整应用场景仍被 `bSupportsFullSceneRendering=false` 门控；不能把阶段 B 验证画面描述为 Vulkan/OpenGL 完整功能对等。

## 优先阅读代码

- `Source/Runtime/RHIVulkan/Private/VulkanDevice.cpp`
- `Source/Runtime/RHIVulkan/Private/VulkanBuffer.cpp`
- `Source/Runtime/RHIVulkan/Private/VulkanTexture.cpp`
- `Source/Runtime/RHIVulkan/Private/VulkanDescriptors.cpp`
- `Source/Runtime/RHIVulkan/Private/VulkanPipeline.cpp`
- `Source/Runtime/Renderer/Private/StaticMeshValidationRenderPath.cpp`
- `Source/Runtime/Renderer/Private/SceneRenderer.cpp`
