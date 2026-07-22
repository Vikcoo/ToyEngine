# Vulkan 后端实施前提与阻塞项处理方案

## 文档目的

本文定义 ToyEngine 开始实现 `RHIVulkan` 之前必须成立的 RHI、Renderer、Engine、Platform 与 Shader 构建前提，并解释每个阻塞项的根因、处理方案、选择理由和验收条件。

本文分为两部分：

- **本轮前置改造**：调整现有公共抽象，并让 `RHIOpenGL` 继续实现同一语义。完成后，项目进入“可以开始 Vulkan Bootstrap”的状态。
- **后续 Vulkan 实施大纲**：只整理阶段与验收目标，本轮不创建完整 Vulkan 后端。前置改造完成后，再按该大纲逐阶段实施。

本文中的“已完成”状态只能在代码、文档和 Release 构建验证同时完成后标记。尚未落地的内容均视为计划中。

## 实施状态

### 前置阻塞处理（2026-07-21）

本轮五项阻塞已经按本文顺序处理完成，项目现在处于“可以开始阶段 A：Vulkan Bootstrap 与清屏”的状态：

- Engine 已改为通过 `RHIDevice::BeginFrame/EndFrame` 获取当前帧命令缓冲并提交呈现，关闭前调用 `WaitIdle()`；窗口不再由 Engine 直接交换缓冲。
- Object、Material、Light、DeferredPass 与 Sky 常量已迁移到按帧分段的 transient uniform ring，通过 `DynamicUniformBuffer + dynamic offset` 绑定。
- Deferred GBuffer 的颜色/深度附件已在写入和采样之间显式执行 `TransitionTexture()`。
- Buffer、Texture、RenderTarget、RenderPass 与 Pipeline 描述已包含显式 API 创建对象所需的 usage、memory、sample、load/store、attachment format 与 blend 信息。
- 主路径 GLSL 已通过公共宏同时表达 group/set 与 binding；OpenGL loader 支持受限递归 include，CMake 在启用 `TE_RHI_VULKAN` 时建立 `VulkanShaders` SPIR-V 目标。
- OpenGL-only Release 全量构建、`RHITransientAllocatorTest`、`MathTest` 与 `SimpleTest` 已通过；Sandbox smoke test 已实际编译并运行 Forward/Sky、Deferred GBuffer/Lighting 全部主路径 Shader/Pipeline。

前置阻塞处理完成时尚未实现任何 `Vk*` 对象、`RHIVulkan` 模块或 Vulkan 运行时路径。该描述保留为阶段历史；后续进展见下一节。

### 阶段 A 完成（2026-07-22）

- 已验证开发机 Vulkan SDK 1.4.350：Headers、`vulkan-1.lib`、`glslc`、Loader、`VK_LAYER_KHRONOS_validation` 与 GLFW Vulkan 支持均可用。当前会话安装后环境变量未刷新，通过显式 `VULKAN_SDK/Vulkan_ROOT/PATH` 完成配置，不需要把 SDK 复制到项目。
- `RHIVulkan` 已成为真实 CMake 模块，并通过公开 `CreateRHIVulkanDevice()` 入口接入 Engine 工厂。CMake 现在要求 OpenGL/Vulkan 恰好选择一个；未实现的 D3D12 选项会明确失败。
- 已实现 Vulkan 1.3 Instance、Debug Messenger、GLFW Surface、Physical/Logical Device、Graphics/Present Queue 与 Dynamic Rendering 特性检查。
- 已实现 Swapchain/ImageView、至少两个 frame slot、Acquire/Submit/Present、resize、最小化恢复、VSync present mode 选择和安全关闭。
- frame slot 持有 CommandPool、CommandBuffer、image-available semaphore 与 submit fence；render-finished semaphore 按交换链图像持有，并用 image-in-flight fence 处理图像与 frame slot 的独立轮转。
- Engine 新增后端场景能力门控。Vulkan 阶段 A 不伪造 Buffer/Pipeline 等资源，而是进入 bootstrap-clear 模式，用统一 RHI RenderPass 调用清除默认交换链目标；OpenGL 保持完整场景路径。
- 已实际生成 7 个 SPIR-V 文件并完成 Validation 运行测试。连续 resize、最小化/恢复和正常关闭无 Validation warning/error；最小化跳过帧时主循环以 16 ms 节流，避免 CPU 空转。OpenGL-only Release 回归、Sandbox 冒烟和全部三个测试目标也已通过。

阶段 A 完成时不包含 Buffer/Image 内存分配、Staging、ShaderModule、Descriptor、Graphics Pipeline、离屏 RenderTarget、transient Uniform 或场景绘制；当时 `bSupportsSceneRendering` 保持 `false`。该描述保留为阶段历史，当前能力见下一节。

### 阶段 B 完成（2026-07-23）

- 已实现 `VulkanBuffer`、`VulkanTexture`、`VulkanSampler` 的基础创建与独立内存分配；GPU-only Buffer 和 Texture 初始数据通过一次性 CommandBuffer 与 Staging Buffer 上传。
- 已实现单 mip `Texture2D`、ImageView、基础资源状态转换与每个交换链图像独立的 `D32_Float` 深度附件。
- 已实现 SPIR-V ShaderModule、DescriptorSetLayout、PipelineLayout、DescriptorPool/Set、dynamic Uniform offset 和 Dynamic Rendering Graphics Pipeline。
- `VulkanCommandBuffer` 已支持 Pipeline、Vertex/Index Buffer、Descriptor Set、Viewport/Scissor、Draw/DrawIndexed 与 Texture Barrier。
- Renderer 新增内部 `FStaticMeshValidationRenderPath`：绘制一个带非对称方向纹理的 indexed cube，覆盖 GPU-only 顶点/索引上传、sRGB 纹理、Sampler、两个 Descriptor Set、per-frame transient Uniform、Reversed-Z、深度写入与正面裁剪。
- 新增 `bSupportsFullSceneRendering` 能力位。Vulkan 阶段 B 允许进入 SceneRenderer，但不会运行 Sandbox 完整 PBR/IBL 场景；OpenGL 保持完整应用场景路径。
- Vulkan Release 已生成 9 个 SPIR-V 文件并完成正常启动、持续绘制与正常关闭；Validation Layer 无 warning/error，运行统计为每帧 1 Draw / 1 Pipeline / 1 VBO / 1 IBO。

阶段 B 当前明确限制为单 `Texture2D`、单 mip、单采样、默认交换链目标和基础图形队列；TextureCube、mip 生成、离屏 RenderTarget/MRT、完整 Forward/Deferred/PBR 场景与通用 fence retirement 属于后续阶段。每资源一次 `vkAllocateMemory` 是正确性优先的阶段实现，不是最终显存分配策略。

## 当前基础与启动时机

当前项目已经具备开始 Vulkan 路线所需的高层基础：

- Renderer 不直接包含 OpenGL 类型，绘制通过 `RHIDevice`、`RHICommandBuffer`、`RHIPipeline` 和 `RHIBindGroup` 提交。
- 已有逻辑 Shader 名称、`BindGroupLayout`、`PipelineLayout`、Forward/Deferred 双路径和 Reversed-Z 深度约定。
- GLFW 窗口能够以 No-API 模式创建，游戏侧与渲染侧数据也已经分离。
- 当前 Shader 数量与 Pass 数量仍然可控，足够验证真实资源流，又没有膨胀到迁移成本不可控。

不应继续等待阴影、后处理或更多材质功能完成后再处理 Vulkan。新增 Pass 会继续依赖 OpenGL 的立即执行、隐式资源状态和窗口直接交换缓冲区，使后续返工面进一步扩大。

## 本轮改造边界

本轮必须完成：

1. 统一帧生命周期与呈现入口。
2. 消除 Renderer 对 OpenGL 立即执行 Uniform 更新语义的依赖。
3. 建立显式纹理状态切换接口，并让 Deferred 主流程声明真实状态变化。
4. 补齐 Vulkan 创建资源与 Pipeline 所需的公共描述信息。
5. 让共享 GLSL 能显式表达 descriptor set/group 与 binding，并建立可选的 SPIR-V 离线编译入口。
6. 保持 OpenGL Forward/Deferred 路径可构建，补充最小回归测试和文档。

本轮明确不做：

- 不实现 `VkInstance`、`VkDevice`、Swapchain 或完整 `RHIVulkan` 类。
- 不引入 RenderGraph、独立 RenderThread、异步 Compute 或多队列调度。
- 不一次性完成 Shader 反射、代码生成和 permutation 系统。
- 不实现 Vulkan 与 OpenGL 的画面功能对等。

这些内容不属于“实施前提”，应在后续 Vulkan 阶段独立验收。

## 阻塞项一：帧获取、提交与呈现不属于 RHI

### 阻塞原因

当前 Engine 创建一个全局 `RHICommandBuffer`，Renderer 自行调用 `Begin/End`，帧尾直接调用 `IWindow::SwapBuffers()`。这种流程只适合 OpenGL：

- OpenGL 的默认帧缓冲由窗口上下文隐式提供。
- Vulkan 必须先 Acquire Swapchain Image，等待 image-available semaphore，再录制对应帧命令。
- 录制后需要 Queue Submit、Fence/Semaphore 同步和 Present。
- Resize、最小化、`OUT_OF_DATE`、`SUBOPTIMAL` 与 VSync/PresentMode 变化都属于 Swapchain 生命周期，不能由窗口的 `SwapBuffers()` 表达。

如果把 Acquire/Submit/Present 偷藏到 `VulkanCommandBuffer::Begin/End`，CommandBuffer 会同时承担命令录制、队列提交和窗口呈现三种职责，后续离屏命令、上传命令或多队列扩展都会被阻塞。

### 实现方案

在 RHI 增加统一的帧级协议：

- `RHIDeviceCreateDesc`：传入后端创建所需的原生窗口句柄、OpenGL 平台呈现回调、初始 VSync 和帧内瞬态资源容量。
- `RHIFrameBeginInfo`：每帧传入当前 framebuffer 尺寸和 VSync 状态。
- `RHIFrameContext`：返回当前帧索引、交换链图像索引和当前帧 `RHICommandBuffer`。
- `RHIFrameStatus`：区分 `Ready`、`Skipped`、`OutOfDate`、`DeviceLost` 和 `Error`。
- `RHIDevice::BeginFrame()` / `EndFrame()`：分别负责帧资源获取与提交呈现。
- `RHIDevice::WaitIdle()`：为 Shutdown、后端切换和必须同步销毁的边界提供显式入口。

Engine 不再持有全局 CommandBuffer，也不再直接 `SwapBuffers()`。Renderer 只消费 `BeginFrame()` 返回的当前帧 CommandBuffer；`renderTarget == nullptr` 继续表示“当前帧呈现目标”，但该语义由帧上下文建立，不再等同于固定的 OpenGL FBO 0。

OpenGL 后端使用单帧 CommandBuffer 并在 `EndFrame()` 调用创建描述中的平台呈现回调。Vulkan 后端未来可在相同接口后管理多帧 CommandPool、CommandBuffer、Fence、Semaphore 和 Swapchain Image。

### 方案理由

- 帧生命周期归属 Device/Queue/Swapchain 侧，而不是窗口或普通 CommandBuffer。
- Engine 只调度统一协议，不根据后端类型分支。
- 保留当前 Renderer 的 `nullptr` 默认目标约定，减少与本次目标无关的 Pass 改写。
- 不提前公开完整 Queue 抽象；当前只有单 Graphics Queue 需求，先保持接口最小，后续多队列阶段再扩展。

### 验收条件

- Engine 不再创建全局 RHI CommandBuffer，不再直接调用 `SwapBuffers()`。
- OpenGL 通过 `RHIDevice::BeginFrame/EndFrame` 完成现有呈现。
- framebuffer 为零时能够返回 `Skipped`，不进入渲染提交。
- Shutdown 在销毁 GPU 资源前调用 `WaitIdle()`。

## 阻塞项二：每 Draw 常量更新依赖立即执行

### 阻塞原因

当前 ObjectBlock 和 MaterialBlock 各自只有一个 UBO。Renderer 在一个循环中反复执行：

```text
UpdateData(同一 Buffer)
SetBindGroup(同一 Buffer)
Draw
```

OpenGL 的 Draw 在函数调用时立即消费当前 Buffer 内容，因此不同对象能够得到不同常量。Vulkan CommandBuffer 只记录“未来使用某个 Buffer 范围”，不会记录普通主机内存更新的历史快照。如果 Vulkan 后端把 `UpdateData()` 实现为持久映射写入，那么提交时多个 Draw 会读取同一范围的最终内容；如果每次更新都单独提交并等待，则正确性依赖昂贵的串行同步。

纹理 BindGroup 也存在相邻风险：当前状态对象在材质变化时重建 BindGroup。显式 API 中，已录制命令引用的 descriptor set 及其原生资源必须存活到 GPU 完成，不能按 OpenGL 立即执行方式立刻回收。

### 实现方案

引入帧内瞬态 Uniform 分配协议：

- `RHIDevice::AllocateTransientUniform()` 接收源数据和大小，返回稳定 Uniform Buffer、帧内唯一 offset 与 range。
- Device 持有一个按 frame segment 划分的 Uniform ring。每个 BeginFrame 只重置当前可安全复用的 segment。
- 分配 offset 必须满足后端 `minUniformBufferOffsetAlignment`；OpenGL 使用项目约定的保守对齐，Vulkan 未来查询物理设备限制。
- 新增 `DynamicUniformBuffer` binding 类型。
- `RHICommandBuffer::SetBindGroup()` 接受 dynamic offsets；Renderer 的 Object/Material/Light/Pass/Sky 常量统一通过瞬态分配上传。
- BindGroup 绑定稳定的 ring buffer，变化的是每次 Draw 的 dynamic offset，不为每个对象创建独立 GPU Buffer。

原生对象销毁需要遵守延迟释放契约：Vulkan descriptor、buffer、image 等原生对象必须在关联 frame fence 完成后真正释放。阶段 B 验证资源常驻到 SceneRenderer 关闭并在销毁前 `WaitIdle()`；通用运行时资源的 frame retirement queue 仍是后续结构性任务，不能把当前直接析构扩展到异步流式资源。

### 方案理由

- 相比“每个对象一个 UBO”，ring buffer 减少小对象和描述符数量。
- 相比 Push Constant，方案不受较小容量上限限制，可统一承载 Object、Material、Light 和 Pass 数据。
- 相比在 `UpdateData()` 内部偷偷提交拷贝命令，dynamic offset 明确表达了帧内数据快照语义，不引入每 Draw GPU 等待。
- 这一设计与显式 API 的 frames-in-flight 模型一致，同时可由 OpenGL 的 `glBindBufferRange` 直接实现。

### 验收条件

- Renderer 主绘制循环不再对单一 Object/Material UBO 原地覆盖。
- 连续两次瞬态分配返回互不重叠且满足对齐的范围。
- `SetBindGroup()` 能把 dynamic offset 映射为 OpenGL `glBindBufferRange`。
- 增加不依赖图形上下文的 ring 分配算法测试，覆盖对齐、溢出和逐帧复用边界。

## 阻塞项三：Pass 之间没有显式资源状态

### 阻塞原因

Deferred GBuffer Pass 把颜色和深度附件作为渲染目标写入，Lighting Pass 随后把同一批纹理作为 Shader Resource 采样。OpenGL 驱动隐式处理了大部分状态与可见性，现有 RHI 因此没有表达：

- `Undefined/ShaderResource -> RenderTarget`
- `Undefined/ShaderResource -> DepthWrite`
- `RenderTarget -> ShaderResource`
- `DepthWrite -> ShaderResource`
- `CopyDest -> ShaderResource`
- `Present <-> RenderTarget`

Vulkan 需要匹配的 image layout、pipeline stage 和 access mask。只在 Vulkan 后端根据调用顺序猜测，会让资源状态成为不可验证的后端隐式规则，也无法支撑后续 RenderGraph。

### 实现方案

- 增加 `RHIResourceState`，覆盖当前实际需要的 `Undefined`、`CopySource`、`CopyDest`、`VertexBuffer`、`IndexBuffer`、`UniformBuffer`、`RenderTarget`、`DepthWrite`、`DepthRead`、`ShaderResource` 和 `Present`。
- 增加 `RHITextureBarrier` 与 `RHICommandBuffer::TransitionTexture()`，由调用方明确给出 before/after。
- Deferred 路径在 GBuffer Pass 前后显式切换所有颜色附件和深度附件。
- Swapchain 的 Present 状态由 `BeginFrame/EndFrame` 内部管理，Renderer 不直接认识 Swapchain Image。
- OpenGL 后端不维护 image layout，但根据状态变化发出必要的 `glMemoryBarrier`，并保留 Debug 状态跟踪以发现 before state 不匹配。

本轮不引入自动 RenderGraph。当前只有少量确定 Pass，手写 barrier 更适合先验证 RHI 状态语义；后续 RenderGraph 可以消费同一 `RHIResourceState`，不需要推翻接口。

### 方案理由

- 状态变化由知道资源用途的 Renderer 声明，后端只做 API 映射。
- 手写状态转换能尽早暴露错误，不把复杂度推迟到 Vulkan 实现阶段。
- 先覆盖当前真实资源流，避免为尚未存在的 Compute、Ray Tracing 或多队列设计过宽接口。

### 验收条件

- Deferred GBuffer 的颜色和深度附件在写入与采样之间都有显式 Transition。
- OpenGL Forward/Deferred 结果不因新接口发生行为变化。
- 文档列出当前所有合法状态转换与后续 Vulkan 映射责任。

## 阻塞项四：资源与 Pipeline 描述不足以创建 Vulkan 对象

### 阻塞原因

当前 Buffer 只有单值 usage，Texture 没有 usage、mip/layer/sample、初始数据布局，Pipeline 也没有输出 attachment formats、blend state 和 sample count。OpenGL 可以从当前绑定状态或纹理创建位置推断这些信息，Vulkan 在创建 Buffer/Image/Pipeline 时必须提前得到完整用途和兼容信息。

### 实现方案

- Buffer usage 改为位标志，补充 `CopySource`、`CopyDest`；增加 `RHIMemoryUsage`，区分 `GPUOnly`、`CPUToGPU`、`GPUToCPU`。
- Texture 增加 usage 位标志、mip levels、array layers、sample count、initial data size 和 row pitch；保留当前 `generateMips` 作为资产上传便利字段。
- RenderTarget 创建的附件自动包含对应 Attachment usage；允许后续采样的附件同时包含 `ShaderResource`。
- Pipeline 增加 color attachment formats、depth/stencil format、sample count 和逐附件 blend/write-mask 描述。
- RenderPass BeginInfo 增加 load/store 行为；当前路径继续使用 Clear/Store，但不再把行为写死在后端。
- Device 提供当前呈现目标 color/depth format 查询，使 Forward、Sky 和 Deferred Lighting Pipeline 能声明与 Swapchain 兼容的输出格式。

### 方案理由

- 描述结构表达“资源将如何使用”，不暴露 `Vk*` 类型。
- 采用位标志允许一个纹理同时作为 RenderTarget 与 ShaderResource，符合 Deferred/后处理真实需求。
- 把 attachment format 放入 Pipeline 描述后，未来可以使用 Vulkan Dynamic Rendering，避免为了兼容性强制引入过早的 RHI RenderPass 对象体系。

### 验收条件

- 所有现有 Buffer、Texture、RenderTarget 和 Pipeline 创建点显式填写或获得正确默认用途。
- OpenGL 后端能够忽略不需要的字段，但不能因描述扩展破坏现有创建行为。
- Forward/Deferred Pipeline 都声明真实输出格式。

## 阻塞项五：Shader 没有 descriptor set 信息和 SPIR-V 构建闭环

### 阻塞原因

当前 GLSL 只有 `layout(binding = N)`。C++ 虽然已经区分 `RendererBindGroups` 和 `RendererBindings`，但 OpenGL 后端忽略 `SetBindGroup(groupIndex)` 中的 groupIndex，只按全局 binding slot 绑定。若直接把现有 GLSL 编译为 Vulkan SPIR-V，资源默认全部进入 descriptor set 0，与 C++ PipelineLayout 的 group 0～4 不一致。

此外，`CMake/ShaderCompile.cmake` 仍是注释示例，没有可执行的 `glslc` 查找、增量编译、输出目录和失败策略。

### 实现方案

- 共享 GLSL 使用统一宏表达 `set/group + binding`。OpenGL 展开为 `layout(binding = bindingIndex)`，Vulkan 展开为 `layout(set = groupIndex, binding = bindingIndex)`。
- 宏定义放入公共 Shader include；OpenGL Shader 加载器实现受限的递归 `#include` 展开，Vulkan 的 `glslc` 使用 include directory 编译相同源文件。
- `ShaderCompile.cmake` 实现真实的 SPIR-V 自定义命令和 `VulkanShaders` 目标，仅在启用 Vulkan 后端时强制要求 `glslc`。OpenGL-only 配置不依赖 Vulkan SDK。
- 逻辑 Shader 名、stage、源文件和输出 `.spv` 的映射集中为 CMake 清单，避免构建脚本继续使用无约束递归 glob。
- 本轮只建立编译闭环和 set/binding 单一写法；正式反射、自动生成 C++ binding 和运行时反射校验仍按独立规划推进。

### 方案理由

- 共享源避免长期维护 OpenGL/Vulkan 两套手写 Shader。
- OpenGL 继续使用运行时 GLSL，不被迫依赖 Vulkan SDK。
- 先让 set/binding 进入 Shader 源级真相，再逐步引入反射，比直接复制 Vulkan Shader 更能控制布局漂移。
- 反射不是“第一个 Vulkan 画面”的硬前提，不应在本轮扩大为完整 Shader 工具链项目。

### 验收条件

- 所有主路径 Shader 通过宏显式声明 group 和 binding。
- OpenGL loader 能正确展开公共 include。
- `TE_RHI_VULKAN=ON` 时存在可执行的 SPIR-V 编译目标；缺少 `glslc` 时在配置阶段给出明确错误。
- OpenGL-only Release 构建不需要 Vulkan SDK。

## 本轮实施顺序

1. 新增帧生命周期类型与 Device 接口，迁移 OpenGL 和 Engine。
2. 新增瞬态 Uniform ring、DynamicUniformBuffer 和 dynamic offsets，迁移 Renderer 常量上传。
3. 新增资源状态与纹理 Barrier，迁移 Deferred Pass。
4. 扩展 Buffer/Texture/Pipeline/RenderPass 描述，补齐现有创建点和 OpenGL 映射。
5. 建立共享 Shader binding 宏、OpenGL include 展开和 SPIR-V CMake 入口。
6. 补算法测试、Release 全量构建、文档与踩坑记录。

顺序理由是先建立帧边界，因为瞬态分配和延迟释放都依赖“当前帧”；再修资源数据与状态语义；最后接 Shader 工具链。每一步都必须保持 OpenGL 可构建，避免把所有错误堆到最后一次集成。

## 后续 Vulkan 实施大纲

前置阻塞完成后，后续按以下阶段推进。每阶段应独立提交、验证和补充架构复盘。

### 阶段 A：Vulkan Bootstrap 与清屏（已完成，2026-07-22）

- 引入 Vulkan Headers/Loader 与 Validation Layer；阶段 A 没有通用 GPU 资源分配，因此内存分配器延后到阶段 B 与 Buffer/Image 一起选择。
- 实现 Instance、Debug Messenger、Surface、PhysicalDevice、LogicalDevice 和 Graphics/Present Queue。
- 实现 Swapchain、两帧 Frames-in-Flight、Acquire/Submit/Present、Resize 与最小化恢复。
- 使用 Dynamic Rendering 完成 Swapchain 清屏。
- 验收结果：启动、连续 resize、最小化恢复和关闭均无 Validation warning/error；Release 全量构建成功。

### 阶段 B：基础资源与静态网格垂直切片（已完成，2026-07-23）

- 实现 Buffer、Image、ImageView、Sampler、内存分配与 Staging Upload。
- 实现 ShaderModule、DescriptorSetLayout、PipelineLayout、DescriptorPool/Set。
- 实现 Graphics Pipeline、顶点/索引绑定和单个静态网格 Forward Draw。
- 验收：Reversed-Z、正面缠绕、纹理方向和颜色空间与 OpenGL 基线一致。

验收结果：Vulkan 使用与 OpenGL 相同的 RH/ZO、CCW、左上纹理原点和 Reversed-Z RHI 契约；阶段 B 非对称纹理用于显式观察 UV/行方向，Graphics Pipeline 在投影 Y 翻转后同步将 RHI CCW 映射为 Vulkan clockwise。Release 构建、SPIR-V 编译、Validation 运行与正常关闭已通过。完整 PBR 画面对等不在本阶段范围内。

### 阶段 C：Forward PBR 与 IBL 对等

- 接通瞬态 Uniform ring、材质 BindGroup、Cubemap、mip 和 IBL 资源。
- 完成 Forward BasePass、Sky、方向光和点光。
- 引入 descriptor 与 pipeline cache，落实 frame fence 延迟释放。
- 验收：多个对象、不同材质和动态相机下不出现常量串写或 descriptor 生命周期错误。

### 阶段 D：Deferred 与显式资源状态对等

- 实现 GBuffer MRT、D32 depth、附件到采样资源的 Barrier。
- 完成 Deferred Lighting 和所有现有调试视图。
- 校准 Depth 重建、RT 原点、NDC Y 和 Reversed-Z。
- 验收：Forward/Deferred 都可运行，F6/F8 深度重建对照符合现有误差基线。

### 阶段 E：稳定性、测试与工具化

- 增加后端选择互斥校验、RHI smoke test、截图回归和 Validation 自动检查入口。
- 完成 DeviceLost、OutOfDate、资源回收、错误日志和 Debug Name。
- 评估 Shader 反射、Pipeline Cache 落盘、RenderDoc 标记和 RenderGraph 的后续优先级。
- 验收：OpenGL/Vulkan 双后端均能通过构建与核心场景回归，文档不再把 Vulkan 标记为占位。

## 总体验收门槛

只有同时满足以下条件，才认为“Vulkan 实施前阻塞项已处理”：

- OpenGL 不再依赖窗口直接呈现和全局 CommandBuffer。
- 每 Draw 常量拥有稳定的帧内快照语义。
- Deferred 资源状态变化在 Renderer 中显式可见。
- RHI 描述足以在不查询 OpenGL 全局状态的情况下创建 Vulkan Buffer/Image/Pipeline。
- Shader 能从同一 set/binding 声明生成 OpenGL GLSL 行为和 Vulkan SPIR-V。
- Release 全量构建通过，瞬态分配算法测试通过。
- `Docs/`、临时架构问题清单和图形踩坑记录与最终代码同步。

上述门槛已于 2026-07-21 满足。受本机未配置 Vulkan SDK / `glslc` 的限制，本轮验证了 OpenGL-only 配置不依赖 SDK，并在独立临时构建目录确认 `TE_RHI_VULKAN=ON` 会因缺少 `glslc` 给出预期的明确配置错误；实际 SPIR-V 产物编译应作为阶段 A 工具链准备的首项验证。
