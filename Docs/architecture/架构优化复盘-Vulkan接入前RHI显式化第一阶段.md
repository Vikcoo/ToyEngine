# 架构优化复盘：Vulkan 接入前 RHI 显式化第一阶段

## 背景与原有问题

ToyEngine 的 Renderer 已经通过 RHI 提交绘制，但旧接口仍建立在 OpenGL 的隐式语义上：窗口直接呈现、CommandBuffer 没有帧归属、每个 Draw 覆盖同一 UBO、Pass 之间没有资源状态、Pipeline 创建描述依赖后端猜测，Shader 也只声明全局 binding。继续直接实现 `RHIVulkan` 会把这些缺口变成后端内部的特殊规则，最终让 OpenGL 与 Vulkan 表面共用接口、实际遵循不同契约。

本阶段因此没有追求尽快创建 `VkInstance`，而是先把公共 RHI 收敛为显式 API 能正确实现的最小语义集合。详细阻塞原因和后续阶段大纲见 [`Vulkan后端实施前提与阻塞项处理方案`](./Vulkan后端实施前提与阻塞项处理方案.md)。

## 第一阶段核心修改（2026-07-21）

### 帧生命周期归属 Device

新增 `RHIDeviceCreateDesc`、`RHIFrameBeginInfo`、`RHIFrameContext` 和 `RHIFrameStatus`，由 `RHIDevice::BeginFrame/EndFrame` 建立当前帧、命令缓冲和呈现边界。OpenGL Device 在 `EndFrame` 调用平台呈现回调；该边界随后已由 Vulkan 阶段 A 用于 Acquire、Submit 与 Present。

选择 Device 作为边界，是因为交换链图像、同步原语和队列提交都属于后端设备/呈现链生命周期。CommandBuffer 只保留命令录制责任，Engine 只保留帧调度责任。

### 常量上传改为帧内快照

新增后端无关的 `RHITransientRangeAllocator` 与 Device transient uniform 分配接口。OpenGL 使用按 frames-in-flight 分段的 Uniform ring，Renderer 的 Object、Material、Light、DeferredPass 和 Sky 数据都分配唯一范围，再通过 `DynamicUniformBuffer` 和 dynamic offset 绑定。

这样处理是为了保证录制式后端看到稳定数据快照。它比“每对象一个 Buffer”更节省对象数量，也比在 `UpdateData()` 内偷偷提交和等待更符合显式 API 的异步执行模型。

### 资源状态和创建契约显式化

RHI 新增资源状态、纹理 barrier、资源 usage/memory、纹理层级与采样数、RenderPass load/store，以及 Pipeline attachment format、blend 和 sample 描述。Deferred 路径明确声明 GBuffer 附件从渲染目标到 ShaderResource 的转换。

本阶段只引入当前 Pass 实际需要的手写状态转换，没有提前加入 RenderGraph。这样既能让 Vulkan 后端机械映射到 layout/stage/access，又保留未来 RenderGraph 消费同一状态枚举的演进路径。

### Shader 的 set/binding 真相进入共享源码

公共 GLSL 宏 `TE_RESOURCE_BINDING` / `TE_UNIFORM_BINDING` 同时接收 group 与 binding：OpenGL 展开时忽略 group，Vulkan SPIR-V 编译时映射为 descriptor set。OpenGL loader 负责展开受限的相对路径 include；CMake 只在 Vulkan 选项启用时查找 `glslc` 并生成显式 Shader 产物清单。

共享源方案避免长期维护两套近似 Shader。当前仍由 GLSL 与 C++ layout 手工保持一致，自动反射和生成属于后续增强，不是第一个 Vulkan 垂直切片的前提。

## 收益

- Vulkan 后端可以按公共描述创建 Buffer、Image 与 Graphics Pipeline，无需通过调用顺序猜测用途。
- Engine、Renderer 与后端的帧责任更清晰，OpenGL 也开始遵守与显式 API 一致的帧协议。
- 多 Draw 常量不再依赖 OpenGL 立即执行，消除了后端迁移后最隐蔽的“所有对象读取最后一次数据”风险。
- Deferred 的跨 Pass 资源流在代码中可见，为 Vulkan barrier 和未来 RenderGraph 建立统一输入。
- OpenGL-only 开发不依赖 Vulkan SDK，同时 Vulkan 模式拥有明确、可失败的 Shader 构建入口。

## 当前限制与下一阶段

- `RHIVulkan` 尚未实现，Device 的 `OutOfDate/DeviceLost` 状态目前只建立了协议。
- OpenGL 的 barrier 只能模拟可见性并做状态校验，不能替代 Vulkan 精确的 stage/access/layout 映射。
- Vulkan 的 descriptor pool、frame fence、延迟释放队列和 transient ring 原生实现仍待完成。
- 本机未配置 `glslc`，本阶段没有生成实际 `.spv`；阶段 A 必须先安装/定位 Vulkan SDK 并验证 `VulkanShaders` 目标。
- Shader 反射、PipelineLayout 自动一致性校验、permutation 与缓存仍按独立规划推进。

下一阶段应严格从“Vulkan Bootstrap 与清屏”开始，只验证 Instance/Device/Surface/Swapchain、两帧同步、resize 和 Dynamic Rendering 清屏，不提前混入静态网格或 PBR，以便把设备与呈现问题限制在最小垂直切片中。

## 验证结果

- CLion MinGW Release 全量配置与构建通过，OpenGL-only 模式明确跳过 SPIR-V 且不要求 Vulkan SDK。
- `RHITransientAllocatorTest` 覆盖初始化、对齐、溢出、帧段隔离和索引回绕；`MathTest`、`SimpleTest` 同步通过。
- Sandbox 隐藏 smoke test 中，Forward/Sky 与切换后的 Deferred GBuffer/Lighting Shader 均成功编译，Pipeline 成功创建，帧循环稳定提交。
- 独立临时配置验证 `TE_RHI_VULKAN=ON` 在本机按预期失败于 `glslc` 缺失，并输出 Vulkan SDK 安装提示；临时构建目录已清理。

## 后续状态：阶段 A 已落地（2026-07-22）

以上“当前限制与下一阶段”记录的是 2026-07-21 的阶段边界，保留用于追溯决策演进。Vulkan SDK 配置后，`RHIVulkan` 已按计划完成 Instance/Device/Surface/Swapchain、双帧同步、resize/minimize 恢复与 Dynamic Rendering 清屏，实际 SPIR-V 构建和 Validation 运行均通过。Device 的 `OutOfDate` 状态现已由交换链路径消费，但 `DeviceLost` 仍只做状态上报。

阶段 A 没有改变本复盘的核心结论：descriptor pool、通用资源的 fence retirement、Vulkan transient ring、ShaderModule/Pipeline 和完整场景仍属于后续阶段。阶段 A 的独立设计与验证复盘见 [`架构优化复盘-Vulkan后端阶段A`](./架构优化复盘-Vulkan后端阶段A.md)。

## 后续状态：阶段 B 已落地（2026-07-23）

阶段 B 已实现基础 Buffer/Image/Sampler、同步 Staging Upload、DescriptorPool/Set、Vulkan transient Uniform ring、ShaderModule 与 Graphics Pipeline，并通过 Renderer 内部静态网格垂直切片实际消费上述前置契约。这证明本复盘建立的 resource usage、attachment format、dynamic offset 和 set/binding 描述能够落到显式后端。

仍未完成的是通用资源 fence retirement、显存子分配、批量上传、离屏 RenderTarget/MRT 和完整应用场景。阶段 B 的独立设计与限制见 [`架构优化复盘-Vulkan后端阶段B`](./架构优化复盘-Vulkan后端阶段B.md)。
