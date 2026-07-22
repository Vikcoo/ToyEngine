# Vulkan 基础资源与 Descriptor 垂直切片

## 解决的问题

本文记录阶段 B 如何把 RHI 的资源描述转换为 Vulkan Buffer/Image、内存、上传、Descriptor 和 Graphics Pipeline，并解释这些对象为何必须按依赖顺序创建与销毁。

## 资源与上传数据流

GPU-only Buffer 的启动期数据流为：

```text
CPU data -> host-visible staging buffer -> vkCmdCopyBuffer -> device-local buffer
```

Texture2D 的数据流为：

```text
CPU rows -> 紧密重排的 staging buffer
         -> UNDEFINED -> TRANSFER_DST_OPTIMAL
         -> vkCmdCopyBufferToImage
         -> SHADER_READ_ONLY_OPTIMAL
```

`initialDataRowPitch` 可以大于紧密行宽，上传前逐行复制，避免把源数据 padding 错当成 texel。阶段 B 只接受一个 mip；上传结束后 Texture 跟踪的 layout 必须与 Descriptor 中声明的 image layout 一致。

CPUToGPU Buffer 使用 host-visible + coherent 内存并保持映射。transient Uniform ring 按 frame slot 划分，每帧只重置已由 fence 确认安全的段；Descriptor 绑定 ring 起点，Draw 通过 Vulkan dynamic offset 选择本次常量快照。

当前 RHI Buffer 只有 `UpdateData()`，还没有 readback/invalidate API，因此阶段 B 的 `GPUToCPU` 也暂按 host-visible + coherent 创建，只保证既有写入接口语义；正式高性能回读需要独立补充读取、invalidate 与 fence 契约。

## Descriptor 与 Pipeline 依赖

创建顺序是：

```text
Buffer / Texture / Sampler
  -> DescriptorSetLayout
  -> PipelineLayout
  -> DescriptorSet
  -> Graphics Pipeline
  -> Bind + Draw
```

销毁时反向执行。PipelineLayout 创建后仍要求关联的 DescriptorSetLayout 在其生命周期内有效；DescriptorSet 引用的 Buffer/Image/Sampler 必须至少活到 GPU 完成对应提交。当前阶段验证资源常驻至 SceneRenderer 销毁，并在销毁前 `WaitIdle()`；通用运行时资源仍需要后续 fence retirement。

Vulkan 的 combined image sampler 对应当前 RHI `Texture2D` binding：一个 entry 同时提供 ImageView、当前 ShaderRead layout 和 Sampler。动态 Uniform 使用 `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`，`vkCmdBindDescriptorSets` 的 dynamic offset 必须满足设备 `minUniformBufferOffsetAlignment` 且能用 32 位值表达。

## Dynamic Rendering Pipeline

Graphics Pipeline 创建时通过 `VkPipelineRenderingCreateInfo` 声明颜色与深度格式，不创建传统 VkRenderPass。交换链颜色格式来自 `GetBackBufferColorFormat()`，深度格式为 `D32_Float`。开始绘制时以 `VkRenderingAttachmentInfo` 绑定当前交换链 ImageView 与同 image index 的深度 ImageView。

项目使用 Reversed-Z：near=1、far=0、depth clear=0、compare=`Greater`。Vulkan 投影矩阵翻转 Y 后会改变 winding，因此 Pipeline 同步反转 native front face；不要再使用负 viewport height，否则会形成第二次方向补偿。

## 当前方案为何不是最终形态

阶段 B 每个资源独立分配内存、每次启动期上传提交后等待 Queue idle、DescriptorPool 使用固定容量。这些选择降低了首个垂直切片的状态复杂度，但不适合大量资产和运行时流送。后续应演进为：

- device-local 大块内存子分配；
- 帧级 upload ring 与批量 copy；
- 基于 fence/timeline 的 staging 与资源延迟释放；
- DescriptorPool 分页和 frame retirement；
- mip、array/cube、MRT 与离屏 RenderTarget。

## 排查优先级

1. 资源 usage 是否包含 copy source/destination 与真实使用位。
2. memory type 是否同时满足 requirements bits 和 property flags。
3. Texture 当前 layout 是否与 Descriptor image layout 一致。
4. dynamic offset 是否对齐、是否落在 ring 对应 frame segment 内。
5. Shader `(set,binding)`、DescriptorSetLayout、PipelineLayout 是否一致。
6. Dynamic Rendering 的 color/depth format 是否与 Pipeline 创建描述一致。
7. 销毁前是否已确认 GPU 完成，Descriptor 引用资源是否仍存活。
