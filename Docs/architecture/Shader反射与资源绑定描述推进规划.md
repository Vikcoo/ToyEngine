# Shader 反射与资源绑定描述推进规划

## 背景与目标

当前 Renderer 已经引入 `BindGroupLayout` / `PipelineLayout`，并把 BindGroup 的 group index 与 shader 内的 binding slot 拆开维护。这为 Vulkan / D3D12 资源绑定模型打下了基础，但资源布局仍然同时写在两处：

- GLSL shader 内的 `layout(binding = N)`。
- C++ 侧的 `RendererBindings`、`RendererBindGroups` 与手写 `RHIBindGroupLayoutDesc`。

这会让 Forward、Deferred、PBR 材质贴图、GBuffer、IBL、后处理和后续阴影系统越做越容易发生布局漂移。目标不是让 shader 反射替代所有渲染资源系统，而是让 GPU 接口形状逐步拥有单一真相源：

- binding slot、资源类型、stage visibility、uniform block、sampler / texture 类型由 shader 编译或反射流程提取。
- 材质语义、资源生命周期、默认资源、缓存策略、颜色空间和 RenderPath 组织仍由 C++ 渲染系统负责。

本文描述的是计划中的推进步骤，当前仓库尚未实现正式 shader 反射、离线编译、生成代码或跨后端 shader 资产管线。

## 推进原则

1. 先校验，再生成，最后再驱动运行时创建。
2. 优先做离线流程，不在每次运行时解析 shader 文本。
3. 反射只负责 GPU 接口描述，不接管高层资源语义。
4. C++ 侧保留显式架构边界，避免 Renderer 退化成读取 shader 元数据的脚本执行器。
5. 每个阶段都应能独立验证，并保持 OpenGL 路径可运行。

## 阶段一：整理资源绑定清单（计划中）

### 目标

把当前散落在 shader 和 C++ 中的绑定约定整理成明确清单，作为后续自动校验的输入基线。

### 步骤

1. 列出所有逻辑 shader 名称与 OpenGL GLSL 文件映射。
2. 为每个 shader 记录：
   - shader stage
   - uniform block 名称与 binding
   - sampler / texture 名称、类型与 binding
   - vertex input location
   - fragment output location
3. 列出 C++ 侧当前对应位置：
   - `RendererBindingSlots.h`
   - Forward / Deferred 中创建 `RHIBindGroupLayoutDesc` 的函数
   - 创建 `RHIPipelineLayoutDesc` 的位置
4. 明确哪些 binding 是跨 pass 共享约定，哪些只属于某个 shader。

### 产出

- 一份人工维护的初始资源绑定清单。
- 明确当前 binding 命名、分组和 shader 名称是否需要先清理。

当前阶段一清单已记录在 `Docs/reference/Shader资源绑定清单.md`。该清单是后续最小 GLSL 校验和生成工具的初始对照，不代表项目已经具备自动 shader 反射能力。

### 你需要准备的内容

- 是否允许调整现有 binding 编号。
- 是否希望保留当前全局 binding 编号风格，还是改为更接近 Vulkan descriptor set / binding 的分组风格。
- 近期是否准备新增阴影、后处理、材质 permutation 或 Vulkan / D3D12 后端。

### 外界依赖

无强依赖。阶段一可以只靠现有源码和文档完成。

## 阶段二：建立离线反射校验（计划中）

### 目标

在不改变运行时资源创建方式的前提下，先让构建或工具脚本能发现 shader 与 C++ layout 不一致。

### 步骤

1. 增加 shader 反射工具入口，例如 `Tools/ShaderReflect` 或 CMake 脚本调用的外部工具。
2. 对当前 OpenGL GLSL，先支持最小解析：
   - `layout(std140, binding = N) uniform BlockName`
   - `layout(binding = N) uniform sampler2D`
   - `layout(binding = N) uniform samplerCube`
   - `layout(location = N) in / out`
3. 输出中间描述，例如 JSON：
   - shader 逻辑名
   - 原始文件路径
   - stage
   - resources
   - inputs / outputs
4. 增加校验脚本，对比反射结果与 C++ 侧声明。
5. 将校验接入一个显式目标，例如 `ShaderReflectionCheck`，先不默认阻塞所有构建。

### 产出

- `Content/Shaders/CompiledShaders` 或 `Intermediate/Shaders` 下的反射 JSON。
- 一个可手动运行的校验目标。
- 文档记录校验失败时应检查 shader binding 还是 C++ layout。

### 你需要准备的内容

- 选择反射结果落盘目录：建议生成物放在 `Intermediate/Shaders`，可发布资产放在 `Content/Shaders/CompiledShaders`。
- 是否允许引入 `Tools/` 目录存放引擎侧离线工具。
- 是否希望校验目标默认参与 Release 构建，还是先由开发者手动运行。

### 外界依赖

有两种选择：

- 低依赖方案：先写一个项目内最小 GLSL 解析器，只覆盖当前 shader 写法。优点是落地快、无第三方安装；缺点是不能长期覆盖复杂预处理、include、宏和跨语言。
- 正式方案：引入 `glslangValidator` + `SPIRV-Reflect` 或同类工具链。优点是更接近 Vulkan / D3D12 未来路线；缺点是需要管理第三方可执行文件或源码依赖。

阶段二建议先用低依赖方案建立校验闭环，再决定是否升级到 SPIR-V 反射。

## 阶段三：生成 C++ binding 常量（计划中）

### 目标

把 `RendererBindings` 中容易漂移的数字常量改为由反射或清单生成，减少 shader 与 C++ 双写。

### 步骤

1. 设计生成头文件，例如 `RendererBindingSlots.generated.h`。
2. 由阶段二的反射结果或资源绑定清单生成：
   - binding 常量
   - shader 资源名称
   - 可选的 debug name
3. 手写 `RendererBindingSlots.h` 只保留稳定的高层分组语义，或转为包含 generated header。
4. 修改 Forward / Deferred 代码使用生成常量。
5. 保留编译期或启动期校验，避免生成文件过期。

### 产出

- 生成的 C++ binding 常量文件。
- 手写 binding 编号减少。
- shader 改 binding 后，C++ 常量可由工具同步更新。

### 你需要准备的内容

- 是否接受 generated header 进入源码树。
- 生成物是否提交到仓库。若希望普通开发者不依赖工具也能构建，应提交生成物；若希望完全由构建生成，则需要保证工具链稳定可用。
- 命名规则，例如 shader 中 `u_BaseColorTex` 是否映射为 `BaseColorTexture`。

### 外界依赖

如果阶段二采用最小解析器，本阶段无新增外部依赖。如果改为 SPIR-V 反射，则依赖所选 shader 工具链。

## 阶段四：生成或构建 BindGroupLayout / PipelineLayout 描述（计划中）

### 目标

让 C++ 不再手写每个 shader 资源的 `RHIBindGroupLayoutEntry`，而是从生成描述中构建 layout。

### 步骤

1. 新增运行时可读的 shader layout 描述结构，例如：
   - `FShaderResourceBinding`
   - `FShaderBindGroupLayoutDesc`
   - `FShaderPipelineLayoutDesc`
2. 将反射资源映射到 RHI 类型：
   - uniform block -> `RHIBindingType::UniformBuffer`
   - `sampler2D` -> `RHIBindingType::Texture2D`
   - `samplerCube` -> `RHIBindingType::TextureCube`
3. 建立 group index 分配规则：
   - pass / object / light / material / environment / gbuffer 等高层分组仍由 Renderer 决定。
   - binding slot 从 shader 反射或生成常量获得。
4. 逐步替换 `CreateMaterialTexturesLayout`、`CreateEnvironmentTexturesLayout`、`CreateGBufferTexturesLayout` 等手写函数。
5. Pipeline 创建时校验 shader 反射资源与传入 PipelineLayout 是否兼容。

### 产出

- Renderer 创建 layout 的重复手写代码减少。
- shader 增删资源时，layout 能跟随生成描述更新或明确报错。
- Vulkan / D3D12 后端接入前获得更稳定的 PipelineLayout 来源。

### 你需要准备的内容

- group index 是否继续由当前 `RendererBindGroups` 固定。
- 是否需要支持一个 shader resource 显式标注所属 group。GLSL 当前只有 `binding`，没有 Vulkan 风格的 `set`；如果要支持 set / group，需要引入新约定。
- 是否接受给 GLSL 增加注释元数据，例如 `// @group MaterialTextures`，作为 OpenGL 阶段的过渡方案。

### 外界依赖

若仍停留在 OpenGL GLSL，可能需要自定义元数据约定来表达 group index。若转向 SPIR-V，则可以使用 `set` / `binding` 反射结果，但 OpenGL 运行路径需要同步决定 shader 源如何书写和交叉编译。

## 阶段五：接入正式 shader 编译与跨后端资产管线（计划中）

### 目标

为 Vulkan / D3D12 做准备，把 shader 从“运行时读取 OpenGL GLSL 文件”推进到“离线编译、反射、缓存、按后端分发”的资产管线。

### 步骤

1. 确定项目主 shader 源语言：
   - 继续以 GLSL 为主。
   - 改为 HLSL 为主，再交叉编译到 SPIR-V / GLSL。
   - 使用一套受限公共 shader 写法，并按后端生成目标代码。
2. 建立编译流程：
   - 源 shader -> SPIR-V
   - SPIR-V -> 反射 JSON
   - SPIR-V -> Vulkan 字节码
   - SPIR-V 或 HLSL -> D3D12 DXIL / DXBC
   - SPIR-V -> OpenGL GLSL 或继续保留 OpenGL 源路径
3. 建立 shader 资产清单：
   - 逻辑 shader 名称
   - stage
   - 源文件
   - 编译产物
   - 反射产物
   - permutation key
4. 将 `OpenGLShaderLibrary` 的手写映射逐步替换为资产清单读取或生成表。
5. 将反射结果作为 PipelineLayout 校验或生成输入。

### 产出

- 正式 shader 编译目标。
- 可缓存的后端 shader 产物。
- 跨后端共享的 shader 资源布局描述。
- 后续接入 Vulkan / D3D12 时不再依赖手工分发 shader 文件。

### 你需要准备的内容

- 确认主 shader 语言路线。
- 确认是否优先接 Vulkan。如果是，SPIR-V 反射优先级更高。
- 确认第三方工具引入方式：源码进 `ThirdParty`、系统安装、还是下载预编译工具。
- 确认生成产物是否提交仓库，以及 CI / 本地构建是否都必须能生成。

### 外界依赖

正式阶段通常需要外部工具：

- `glslangValidator`：GLSL/HLSL 到 SPIR-V。
- `SPIRV-Reflect` 或 `SPIRV-Cross`：提取资源绑定与生成跨后端代码。
- D3D12 路线可能需要 `dxc`。
- 如果做 shader 格式校验和优化，可能继续引入 `spirv-tools`。

这些依赖可以先作为可选工具，不必在 OpenGL 单后端阶段一次性全部接入。

## 阶段六：运行时消费反射数据（计划中）

### 目标

让运行时 Pipeline 创建、资源绑定校验、调试日志和错误提示都能使用反射数据。

### 步骤

1. RHI Shader 对象或 Renderer shader 资产对象保存反射摘要。
2. Pipeline 创建时校验：
   - shader 需要的资源是否都在 PipelineLayout 中声明。
   - 绑定类型是否一致。
   - stage visibility 是否覆盖实际使用阶段。
3. BindGroup 创建时校验：
   - binding 是否存在。
   - resource type 是否匹配。
   - 必需资源是否缺失。
4. 日志中输出逻辑 shader 名称、资源名、binding、group 和类型，方便排查。
5. Debug 构建可开启严格校验，Release 构建保留低成本必要检查。

### 产出

- 资源绑定错误更早暴露。
- 调试时能直接看到 shader 期望资源与 C++ 实际资源的差异。
- 后续多后端接入时，PipelineLayout 不一致问题更容易定位。

### 你需要准备的内容

- 是否接受 Debug / Release 校验强度不同。
- 错误策略：创建失败立即中止，还是降级为日志警告并跳过 draw。
- 是否需要在 Sandbox 中增加一个专门的 shader binding 校验示例。

### 外界依赖

运行时消费反射数据本身不需要外部依赖，但依赖前面阶段生成稳定的反射产物。

## 推荐落地顺序

短期推荐顺序：

1. 阶段一：整理资源绑定清单。
2. 阶段二：最小 GLSL 反射校验。
3. 阶段三：生成 C++ binding 常量。
4. 阶段四：用生成描述创建 BindGroupLayout。

中长期推荐顺序：

1. 确认 Vulkan / D3D12 优先级。
2. 决定主 shader 语言。
3. 接入 `glslangValidator`、`SPIRV-Reflect` 或 `SPIRV-Cross`。
4. 建立正式 shader 资产清单、离线编译、反射、permutation 与后端产物缓存。

## 当前不建议立即做的事

- 不建议直接让运行时每次启动解析所有 shader 文本并驱动资源布局创建。
- 不建议在 OpenGL 单后端阶段一次性引入完整 Vulkan / D3D12 shader 工具链并强制所有构建依赖它。
- 不建议让 shader 反射决定材质语义、纹理颜色空间、默认贴图或资源生命周期。
- 不建议继续长期手写新增 shader 的 binding 数字而没有任何校验。

## 对用户的准备清单

如果要按本文推进，需要先确认以下事项：

1. 后端路线：近期是否优先推进 Vulkan / D3D12。
2. Shader 语言路线：继续 GLSL，还是转向 HLSL / SPIR-V 主流程。
3. 生成物策略：反射 JSON 和 generated header 是否提交仓库。
4. 工具依赖策略：先项目内最小解析器，还是直接引入 `glslangValidator` / `SPIRV-Reflect`。
5. Binding 分组策略：是否继续沿用当前 `RendererBindGroups`，以及 OpenGL GLSL 如何表达 group。
6. 构建接入策略：shader 校验是默认构建必跑，还是先作为手动目标。

在这些决策明确前，最稳妥的第一步是先做阶段一和阶段二的低依赖版本：整理清单并增加最小校验目标。
