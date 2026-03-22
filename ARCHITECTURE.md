# ToyEngine 架构设计文档

> 一个借鉴 UE5 架构思想的简易 3D 游戏引擎，用于学习和理解引擎底层原理。
> 当前目标：实现一个模型的 PBR 渲染，支持前向渲染和延迟渲染。

---

## 目录

1. [架构总览](#1-架构总览)
2. [目录结构](#2-目录结构)
3. [模块详细设计](#3-模块详细设计)
4. [第三方库清单](#4-第三方库清单)
5. [Shader 跨后端策略](#5-shader-跨后端策略)
6. [技术实施路线](#6-技术实施路线)
7. [关键架构决策](#7-关键架构决策)
8. [当前代码问题与改进建议](#8-当前代码问题与改进建议)
9. [参考资料](#9-参考资料)

---

## 1. 架构总览

### 1.1 设计原则

- **借鉴 UE5，但不照搬**：采用 UE5 的 Actor-Component 模式和 RHI 分层思想，但大幅简化
- **图形 API 无关**：RHI 层为纯抽象接口，后端可插拔（Vulkan / OpenGL / D3D12）
- **可扩展**：为动画、物理、2D 渲染、UI 等未来功能预留接口
- **易于理解**：代码清晰，注释充分，适合学习

### 1.2 模块依赖关系

```
┌─────────────────────────────────────────────────────────┐
│                       Sandbox / Editor                  │  ← 应用层
├─────────────────────────────────────────────────────────┤
│                         Engine                          │  ← 引擎主循环
├──────────┬──────────┬──────────┬────────────────────────┤
│  Scene   │ Renderer │  Asset   │    Input (可选)        │  ← 功能模块
├──────────┴──────────┴──────────┴────────────────────────┤
│                     CoreObject                          │  ← 对象系统
├─────────────────────────────────────────────────────────┤
│              RHI (纯抽象接口)                             │  ← 图形抽象层
├──────────┬──────────┬───────────────────────────────────┤
│ VulkanRHI│ OpenGLRHI│  D3D12RHI                         │  ← 图形后端(选其一)
├──────────┴──────────┴───────────────────────────────────┤
│                      Platform                           │  ← 平台层(窗口/输入)
├─────────────────────────────────────────────────────────┤
│                        Core                             │  ← 基础设施
│              (Memory / Log / Math / Misc)               │
└─────────────────────────────────────────────────────────┘
```

### 1.3 数据流（每帧）

```
Input → Engine::Tick(DeltaTime)
          │
          ├─ 1. Platform::PollEvents()          // 处理窗口/输入事件
          ├─ 2. World::Tick(DeltaTime)          // 遍历所有 Actor/Component 的 Tick
          ├─ 3. SceneRenderer::Render(World)    // 从 World 收集可渲染对象，提交到 RHI
          │       ├─ 收集 MeshComponent → 构建 RenderScene
          │       ├─ 收集 LightComponent → 构建光源列表
          │       ├─ 获取 CameraComponent → 构建 ViewInfo
          │       └─ 执行渲染管线（Forward 或 Deferred）
          └─ 4. RHI::Present()                  // 交换链呈现
```

---

## 2. 目录结构

```
ToyEngine/
├── CMakeLists.txt                          # 根 CMake 配置
├── ARCHITECTURE.md                         # 本文档
├── CMake/
│   ├── EngineOptions.cmake                 # 引擎构建选项（后端选择等）
│   └── CompileShaders.cmake                # 着色器编译脚本
│
├── Content/                                # 资源目录（不参与编译）
│   ├── Config/
│   │   └── Engine.ini                      # 引擎配置（包括 RHI 后端选择）
│   ├── Models/                             # 3D 模型文件
│   ├── Textures/                           # 纹理文件
│   └── Shaders/
│       ├── Common/                         # 公共 shader include（光照函数等）
│       ├── Forward/                        # 前向渲染 shader
│       ├── Deferred/                       # 延迟渲染 shader
│       ├── PBR/                            # PBR 相关函数
│       └── PostProcess/                    # 后处理 shader
│
├── Source/
│   ├── Runtime/
│   │   ├── Core/                           # [必须] 基础设施模块
│   │   │   ├── Public/
│   │   │   │   ├── Log/Log.h
│   │   │   │   ├── Memory/
│   │   │   │   │   ├── Memory.h
│   │   │   │   │   └── MemoryTag.h
│   │   │   │   ├── Math/
│   │   │   │   │   └── MathTypes.h         # glm 类型别名封装
│   │   │   │   └── Misc/
│   │   │   │       └── PathUtils.h         # 路径工具
│   │   │   └── Private/
│   │   │
│   │   ├── CoreObject/                     # [必须] 对象系统
│   │   │   ├── Public/
│   │   │   │   ├── Object.h               # TObject 基类
│   │   │   │   ├── Class.h                # TClass 类型信息
│   │   │   │   └── ObjectMacros.h         # TE_DECLARE_CLASS 等宏
│   │   │   └── Private/
│   │   │       ├── Object.cpp
│   │   │       └── Class.cpp
│   │   │
│   │   ├── Engine/                         # [必须] 引擎核心
│   │   │   ├── Public/
│   │   │   │   ├── Engine.h               # 引擎主类
│   │   │   │   └── EngineLoop.h           # 主循环封装
│   │   │   └── Private/
│   │   │       ├── Engine.cpp
│   │   │       └── EngineLoop.cpp
│   │   │
│   │   ├── Platform/                       # [必须] 平台抽象（已有）
│   │   │   ├── Public/
│   │   │   │   └── Window.h
│   │   │   └── Private/
│   │   │       ├── GLFW/
│   │   │       │   ├── GLFWWindow.h
│   │   │       │   └── GLFWWindow.cpp
│   │   │       └── PlatformFactory.cpp
│   │   │
│   │   ├── RHI/                            # [必须] 渲染硬件接口（纯抽象）
│   │   │   ├── Public/
│   │   │   │   ├── RHI.h                  # 总入口 + ERHIBackend 枚举
│   │   │   │   ├── RHIDevice.h            # 设备接口
│   │   │   │   ├── RHISwapChain.h         # 交换链接口
│   │   │   │   ├── RHICommandBuffer.h     # 命令缓冲接口
│   │   │   │   ├── RHIBuffer.h            # Buffer 接口
│   │   │   │   ├── RHITexture.h           # 纹理接口
│   │   │   │   ├── RHIPipeline.h          # 管线接口
│   │   │   │   ├── RHIShader.h            # Shader 接口
│   │   │   │   ├── RHIRenderPass.h        # 渲染通道接口
│   │   │   │   └── RHIDescriptor.h        # 描述符接口
│   │   │   └── Private/
│   │   │       └── RHIFactory.cpp         # 后端工厂（动态选择）
│   │   │
│   │   ├── VulkanRHI/                      # [可选后端] Vulkan 实现
│   │   │   ├── Public/
│   │   │   │   └── VulkanRHI.h
│   │   │   └── Private/
│   │   │       ├── VulkanDevice.cpp
│   │   │       ├── VulkanSwapChain.cpp
│   │   │       ├── VulkanCommandBuffer.cpp
│   │   │       ├── VulkanBuffer.cpp
│   │   │       ├── VulkanTexture.cpp
│   │   │       ├── VulkanPipeline.cpp
│   │   │       ├── VulkanShader.cpp
│   │   │       ├── VulkanRenderPass.cpp
│   │   │       └── VulkanDescriptor.cpp
│   │   │
│   │   ├── OpenGLRHI/                      # [可选后端] OpenGL 实现
│   │   │   ├── Public/
│   │   │   │   └── OpenGLRHI.h
│   │   │   └── Private/
│   │   │
│   │   ├── D3D12RHI/                       # [可选后端] DirectX 12 实现
│   │   │   ├── Public/
│   │   │   │   └── D3D12RHI.h
│   │   │   └── Private/
│   │   │
│   │   ├── Renderer/                       # [必须] 高级渲染
│   │   │   ├── Public/
│   │   │   │   ├── SceneRenderer.h        # 场景渲染器（调度中心）
│   │   │   │   ├── ForwardRenderer.h      # 前向渲染管线
│   │   │   │   ├── DeferredRenderer.h     # 延迟渲染管线
│   │   │   │   ├── RenderScene.h          # 渲染场景数据（从 World 收集）
│   │   │   │   ├── Camera.h              # 相机数据
│   │   │   │   ├── Light.h               # 灯光定义
│   │   │   │   └── Material.h            # PBR 材质
│   │   │   └── Private/
│   │   │       ├── SceneRenderer.cpp
│   │   │       ├── ForwardRenderer.cpp
│   │   │       ├── DeferredRenderer.cpp
│   │   │       └── Material.cpp
│   │   │
│   │   ├── Scene/                          # [必须] 场景系统
│   │   │   ├── Public/
│   │   │   │   ├── World.h               # TWorld（场景容器）
│   │   │   │   ├── Actor.h               # TActor（场景实体）
│   │   │   │   ├── Component.h           # TComponent 基类
│   │   │   │   ├── SceneComponent.h      # TSceneComponent（带 Transform）
│   │   │   │   ├── MeshComponent.h       # TMeshComponent（可渲染）
│   │   │   │   ├── LightComponent.h      # TLightComponent
│   │   │   │   └── CameraComponent.h     # TCameraComponent
│   │   │   └── Private/
│   │   │       ├── World.cpp
│   │   │       ├── Actor.cpp
│   │   │       ├── SceneComponent.cpp
│   │   │       ├── MeshComponent.cpp
│   │   │       ├── LightComponent.cpp
│   │   │       └── CameraComponent.cpp
│   │   │
│   │   ├── Asset/                          # [必须] 资源系统
│   │   │   ├── Public/
│   │   │   │   ├── AssetManager.h        # 资源管理器
│   │   │   │   ├── Mesh.h               # 网格数据
│   │   │   │   ├── Texture.h            # 纹理数据
│   │   │   │   └── Shader.h             # Shader 资源
│   │   │   └── Private/
│   │   │       ├── AssetManager.cpp
│   │   │       ├── MeshLoader.cpp        # OBJ/glTF 加载
│   │   │       └── TextureLoader.cpp     # stb_image 加载
│   │   │
│   │   └── Input/                          # [可选] 输入系统
│   │       ├── Public/
│   │       │   └── InputSystem.h
│   │       └── Private/
│   │           └── InputSystem.cpp
│   │
│   ├── Editor/                             # [可选] 编辑器（未来扩展）
│   └── Sandbox/                            # 测试应用
│       ├── CMakeLists.txt
│       └── Main.cpp
│
├── Tests/
│   ├── CMakeLists.txt
│   └── SimpleTest.cpp
│
└── ThirdParty/
    ├── CMakeLists.txt
    ├── glfw-3.4/          # 窗口管理
    ├── glm/               # 数学库
    ├── spdlog/            # 日志库
    ├── tlsf/              # CPU 内存分配
    ├── stb-master/        # 图像加载（stb_image）
    └── tinyobjloader-release/  # OBJ 模型加载
```

---

## 3. 模块详细设计

### 3.1 Core — 基础设施 [必须]

> 对应 UE5 的 `Core` 模块，提供内存、日志、数学、工具函数等最底层功能。

**已有功能：**
- `Memory` — TLSF 内存分配器封装，支持 Tag 统计、对齐分配、自动扩容
- `Log` — 基于 spdlog 的异步日志系统

**需要新增：**

#### 3.1.1 Math 封装 [必须]

将 glm 类型封装为引擎类型别名，统一全引擎的数学类型使用：

```cpp
// Core/Public/Math/MathTypes.h
#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace TE {

// 向量
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;

// 矩阵
using Matrix3 = glm::mat3;
using Matrix4 = glm::mat4;

// 四元数
using Quat = glm::quat;

// 变换（位置 + 旋转 + 缩放）
struct Transform
{
    Vector3 Position = Vector3(0.0f);
    Quat    Rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vector3 Scale    = Vector3(1.0f);

    Matrix4 ToMatrix() const
    {
        Matrix4 mat(1.0f);
        mat = glm::translate(mat, Position);
        mat *= glm::mat4_cast(Rotation);
        mat = glm::scale(mat, Scale);
        return mat;
    }
};

} // namespace TE
```

#### 3.1.2 Misc 工具 [可选]

- `PathUtils` — 项目根目录获取、资源路径拼接
- `StringUtils` — 字符串哈希、格式化工具

---

### 3.2 CoreObject — 对象系统 [必须]

> 简化版 UE5 `UObject`。提供类型信息和统一的基类，但不实现完整 GC 和反射。

#### 3.2.1 TClass — 运行时类型信息

```cpp
// CoreObject/Public/Class.h
#pragma once
#include <string>
#include <functional>
#include <unordered_map>

namespace TE {

class TObject;

class TClass
{
public:
    using FactoryFunc = std::function<TObject*()>;

    TClass(const std::string& name, TClass* parent, FactoryFunc factory);

    const std::string& GetName() const { return m_Name; }
    TClass* GetParent() const { return m_Parent; }

    bool IsChildOf(const TClass* other) const;
    TObject* CreateInstance() const;

    // 全局类型注册表
    static TClass* FindClass(const std::string& name);
    static void RegisterClass(TClass* cls);

private:
    std::string m_Name;
    TClass* m_Parent = nullptr;
    FactoryFunc m_Factory;

    static std::unordered_map<std::string, TClass*>& GetRegistry();
};

} // namespace TE
```

#### 3.2.2 TObject — 引擎对象基类

```cpp
// CoreObject/Public/Object.h
#pragma once
#include "Class.h"
#include <string>
#include <cstdint>

namespace TE {

class TObject
{
public:
    virtual ~TObject() = default;

    const std::string& GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

    uint32_t GetObjectId() const { return m_ObjectId; }

    virtual TClass* GetClass() const = 0;

    template<typename T>
    bool IsA() const { return GetClass()->IsChildOf(T::StaticClass()); }

protected:
    std::string m_Name;
    uint32_t m_ObjectId = 0;

    static uint32_t GenerateObjectId();
};

} // namespace TE
```

#### 3.2.3 类型注册宏

```cpp
// CoreObject/Public/ObjectMacros.h
#pragma once

// 在类声明内使用：声明 StaticClass() 和 GetClass()
#define TE_DECLARE_CLASS(ClassName, ParentClass)                             \
public:                                                                      \
    static TE::TClass* StaticClass()                                        \
    {                                                                        \
        static TE::TClass s_Class(                                          \
            #ClassName,                                                      \
            ParentClass::StaticClass(),                                     \
            []() -> TE::TObject* { return new ClassName(); }                \
        );                                                                   \
        return &s_Class;                                                     \
    }                                                                        \
    TE::TClass* GetClass() const override { return ClassName::StaticClass(); }

// TObject 自身的特殊版本（没有父类）
#define TE_DECLARE_ROOT_CLASS(ClassName)                                     \
public:                                                                      \
    static TE::TClass* StaticClass()                                        \
    {                                                                        \
        static TE::TClass s_Class(#ClassName, nullptr,                      \
            []() -> TE::TObject* { return nullptr; });                      \
        return &s_Class;                                                     \
    }                                                                        \
    TE::TClass* GetClass() const override { return ClassName::StaticClass(); }
```

---

### 3.3 Engine — 引擎核心 [必须]

> 对应 UE5 的 `FEngineLoop`，管理所有子系统的初始化、Tick 循环和关闭。

```cpp
// Engine/Public/Engine.h
#pragma once
#include <memory>

namespace TE {

class Window;
class IRHIDevice;
class SceneRenderer;
class TWorld;
class AssetManager;

class Engine
{
public:
    static Engine& Get();

    void Init();
    void Run();         // 主循环（阻塞直到退出）
    void Shutdown();

    // 子系统访问
    Window*         GetWindow()    const { return m_Window.get(); }
    IRHIDevice*     GetRHI()       const { return m_RHIDevice.get(); }
    SceneRenderer*  GetRenderer()  const { return m_Renderer.get(); }
    TWorld*         GetWorld()     const { return m_World.get(); }
    AssetManager*   GetAssets()    const { return m_AssetManager.get(); }

private:
    Engine() = default;

    void Tick(float DeltaTime);

    // 子系统（按初始化顺序排列）
    std::unique_ptr<Window>         m_Window;
    std::unique_ptr<IRHIDevice>     m_RHIDevice;
    std::unique_ptr<AssetManager>   m_AssetManager;
    std::unique_ptr<TWorld>         m_World;
    std::unique_ptr<SceneRenderer>  m_Renderer;

    bool m_Running = false;
};

} // namespace TE
```

**初始化顺序**（Shutdown 逆序执行）：

```
1. Log::Init()              // 日志（最先）
2. MemoryInit()             // 内存
3. Window::Create()         // 窗口
4. RHIFactory::CreateDevice() // RHI 设备
5. AssetManager::Init()     // 资源系统
6. World::Init()            // 场景
7. SceneRenderer::Init()    // 渲染器（最后）
```

**主循环伪代码：**

```cpp
void Engine::Run()
{
    m_Running = true;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_Running && !m_Window->ShouldClose())
    {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        Tick(dt);
    }
}

void Engine::Tick(float DeltaTime)
{
    m_Window->PollEvents();           // 1. 事件处理
    m_World->Tick(DeltaTime);         // 2. 逻辑更新
    m_Renderer->Render(m_World.get());// 3. 渲染
}
```

---

### 3.4 Platform — 平台抽象 [必须，已有]

已完成：`Window` 抽象接口 + GLFW 实现。

**未来扩展点：**
- 添加鼠标位置、鼠标按钮回调
- 添加 `GetRequiredInstanceExtensions()` 为 Vulkan 后端提供 surface 扩展

---

### 3.5 RHI — 渲染硬件接口 [必须]

> 对应 UE5 的 `RHI` 模块。**纯抽象接口**，不包含任何图形 API 头文件。
> Renderer 和上层代码只依赖此模块，对具体图形后端完全无感知。

#### 3.5.1 后端选择机制

```cpp
// RHI/Public/RHI.h
#pragma once
#include <memory>

namespace TE {

class Window;

enum class ERHIBackend
{
    Vulkan,
    OpenGL,
    D3D12,
};

class IRHIDevice;

// 后端工厂 — 根据配置创建对应的 RHI 设备
class RHIFactory
{
public:
    static std::unique_ptr<IRHIDevice> CreateDevice(
        ERHIBackend backend,
        Window* window
    );
};

} // namespace TE
```

CMake 层控制编译哪些后端：

```cmake
# CMake/EngineOptions.cmake
option(TE_RHI_VULKAN  "Build Vulkan RHI backend"  OFF)
option(TE_RHI_OPENGL  "Build OpenGL RHI backend"  OFF)
option(TE_RHI_D3D12   "Build D3D12 RHI backend"   OFF)
```

#### 3.5.2 核心抽象接口

以下每个接口都是纯虚类，具体实现由后端模块提供：

```cpp
// RHI/Public/RHIDevice.h
#pragma once
#include <memory>
#include <cstdint>

namespace TE {

struct RHISwapChainDesc;
struct RHIBufferDesc;
struct RHITextureDesc;
struct RHIShaderDesc;
struct RHIGraphicsPipelineDesc;
struct RHIRenderPassDesc;

class IRHISwapChain;
class IRHIBuffer;
class IRHITexture;
class IRHIShader;
class IRHIPipeline;
class IRHIRenderPass;
class IRHICommandBuffer;
class IRHIDescriptorSet;

class IRHIDevice
{
public:
    virtual ~IRHIDevice() = default;

    // 资源创建
    virtual std::unique_ptr<IRHISwapChain>    CreateSwapChain(const RHISwapChainDesc& desc) = 0;
    virtual std::unique_ptr<IRHIBuffer>       CreateBuffer(const RHIBufferDesc& desc) = 0;
    virtual std::unique_ptr<IRHITexture>      CreateTexture(const RHITextureDesc& desc) = 0;
    virtual std::unique_ptr<IRHIShader>       CreateShader(const RHIShaderDesc& desc) = 0;
    virtual std::unique_ptr<IRHIPipeline>     CreateGraphicsPipeline(const RHIGraphicsPipelineDesc& desc) = 0;
    virtual std::unique_ptr<IRHIRenderPass>   CreateRenderPass(const RHIRenderPassDesc& desc) = 0;
    virtual std::unique_ptr<IRHICommandBuffer> CreateCommandBuffer() = 0;

    // 提交与同步
    virtual void Submit(IRHICommandBuffer* cmdBuffer) = 0;
    virtual void WaitIdle() = 0;
};
```

```cpp
// RHI/Public/RHICommandBuffer.h — 命令缓冲接口
#pragma once

namespace TE {

class IRHIRenderPass;
class IRHIPipeline;
class IRHIBuffer;
class IRHIDescriptorSet;

class IRHICommandBuffer
{
public:
    virtual ~IRHICommandBuffer() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    // 渲染通道
    virtual void BeginRenderPass(IRHIRenderPass* renderPass) = 0;
    virtual void EndRenderPass() = 0;

    // 管线绑定
    virtual void BindPipeline(IRHIPipeline* pipeline) = 0;
    virtual void BindVertexBuffer(IRHIBuffer* buffer, uint32_t binding = 0) = 0;
    virtual void BindIndexBuffer(IRHIBuffer* buffer) = 0;
    virtual void BindDescriptorSet(IRHIDescriptorSet* descriptorSet, uint32_t setIndex = 0) = 0;

    // 绘制
    virtual void Draw(uint32_t vertexCount, uint32_t firstVertex = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0) = 0;

    // 视口与裁剪
    virtual void SetViewport(float x, float y, float w, float h) = 0;
    virtual void SetScissor(int32_t x, int32_t y, uint32_t w, uint32_t h) = 0;
};

} // namespace TE
```

```cpp
// RHI/Public/RHIBuffer.h — Buffer 接口
#pragma once
#include <cstddef>
#include <cstdint>

namespace TE {

enum class ERHIBufferUsage : uint32_t
{
    Vertex   = 1 << 0,
    Index    = 1 << 1,
    Uniform  = 1 << 2,
    Storage  = 1 << 3,
    Transfer = 1 << 4,
};

struct RHIBufferDesc
{
    uint64_t       SizeInBytes = 0;
    ERHIBufferUsage Usage      = ERHIBufferUsage::Vertex;
    bool           CPUVisible  = false;   // 是否需要 CPU 可映射
};

class IRHIBuffer
{
public:
    virtual ~IRHIBuffer() = default;

    virtual void* Map() = 0;
    virtual void  Unmap() = 0;
    virtual void  UpdateData(const void* data, uint64_t size, uint64_t offset = 0) = 0;

    virtual uint64_t GetSize() const = 0;
};

} // namespace TE
```

```cpp
// RHI/Public/RHITexture.h — 纹理接口
#pragma once
#include <cstdint>

namespace TE {

enum class ERHITextureFormat : uint32_t
{
    RGBA8_UNORM,
    RGBA8_SRGB,
    RGBA16_FLOAT,
    RGBA32_FLOAT,
    Depth24_Stencil8,
    Depth32_Float,
};

enum class ERHITextureUsage : uint32_t
{
    Sampled       = 1 << 0,   // 可采样
    RenderTarget  = 1 << 1,   // 颜色附件
    DepthStencil  = 1 << 2,   // 深度/模板附件
};

struct RHITextureDesc
{
    uint32_t          Width  = 1;
    uint32_t          Height = 1;
    ERHITextureFormat Format = ERHITextureFormat::RGBA8_UNORM;
    ERHITextureUsage  Usage  = ERHITextureUsage::Sampled;
    uint32_t          MipLevels = 1;
};

class IRHITexture
{
public:
    virtual ~IRHITexture() = default;

    virtual void UploadData(const void* pixels, uint64_t size) = 0;
    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
};

} // namespace TE
```

```cpp
// RHI/Public/RHIPipeline.h — 管线接口
#pragma once
#include <cstdint>
#include <vector>

namespace TE {

class IRHIShader;
class IRHIRenderPass;

enum class ERHIVertexFormat : uint32_t
{
    Float1, Float2, Float3, Float4,
};

struct RHIVertexAttribute
{
    uint32_t          Location = 0;
    uint32_t          Binding  = 0;
    ERHIVertexFormat  Format   = ERHIVertexFormat::Float3;
    uint32_t          Offset   = 0;
};

struct RHIVertexBinding
{
    uint32_t Binding    = 0;
    uint32_t Stride     = 0;
    bool     PerInstance = false;
};

enum class ERHICullMode { None, Front, Back };
enum class ERHIFrontFace { CCW, CW };
enum class ERHICompareOp { Less, LessOrEqual, Greater, Always };

struct RHIGraphicsPipelineDesc
{
    IRHIShader*     VertexShader   = nullptr;
    IRHIShader*     FragmentShader = nullptr;
    IRHIRenderPass* RenderPass     = nullptr;

    std::vector<RHIVertexAttribute> VertexAttributes;
    std::vector<RHIVertexBinding>   VertexBindings;

    ERHICullMode   CullMode    = ERHICullMode::Back;
    ERHIFrontFace  FrontFace   = ERHIFrontFace::CCW;
    bool           DepthTest   = true;
    bool           DepthWrite  = true;
    ERHICompareOp  DepthCompare = ERHICompareOp::Less;
    bool           EnableBlend  = false;
};

class IRHIPipeline
{
public:
    virtual ~IRHIPipeline() = default;
};

} // namespace TE
```

#### 3.5.3 RHI 与后端的关系图

```
上层代码（Renderer / Scene / Sandbox）
           │
           │ 只依赖 RHI 接口
           ▼
    ┌──────────────┐
    │    RHI 模块   │  纯虚接口 + 枚举 + 描述结构体
    │  (无 API 依赖) │  IRHIDevice / IRHIBuffer / ...
    └──────┬───────┘
           │ RHIFactory 根据配置选择后端
           ▼
    ┌──────┼──────┬────────────┐
    │      │      │            │
    ▼      ▼      ▼            ▼
VulkanRHI OpenGLRHI D3D12RHI  (未来后端...)
    │      │      │
    ▼      ▼      ▼
Vulkan API OpenGL  D3D12
```

---

### 3.6 Renderer — 高级渲染 [必须]

> 对应 UE5 的 `Renderer` 模块。只通过 RHI 接口操作 GPU，不直接调用任何图形 API。

#### 3.6.1 SceneRenderer — 场景渲染调度器

```cpp
// Renderer/Public/SceneRenderer.h
#pragma once
#include <memory>

namespace TE {

class IRHIDevice;
class TWorld;
class ForwardRenderer;
class DeferredRenderer;

enum class ERenderPath { Forward, Deferred };

class SceneRenderer
{
public:
    explicit SceneRenderer(IRHIDevice* device);
    ~SceneRenderer();

    void SetRenderPath(ERenderPath path) { m_RenderPath = path; }
    void Render(TWorld* world);

private:
    IRHIDevice* m_Device = nullptr;
    ERenderPath m_RenderPath = ERenderPath::Forward;

    std::unique_ptr<ForwardRenderer>  m_ForwardRenderer;
    std::unique_ptr<DeferredRenderer> m_DeferredRenderer;
};

} // namespace TE
```

#### 3.6.2 前向渲染管线 [必须]

```
ForwardRenderer::Render(RenderScene)
│
├─ 1. BeginFrame         获取交换链图像
├─ 2. BeginRenderPass    颜色 + 深度附件
├─ 3. 遍历可渲染对象:
│     ├─ BindPipeline(PBR Forward Pipeline)
│     ├─ BindDescriptorSet(相机UBO + 光源UBO + 材质纹理)
│     ├─ BindVertexBuffer / BindIndexBuffer
│     └─ DrawIndexed
├─ 4. EndRenderPass
└─ 5. Present            提交并呈现
```

#### 3.6.3 延迟渲染管线 [推荐]

```
DeferredRenderer::Render(RenderScene)
│
├─ GBuffer Pass（多 RenderTarget 输出）:
│   ├─ RT0: Albedo.rgb + Metallic.a        (RGBA8)
│   ├─ RT1: WorldNormal.rgb + Roughness.a  (RGBA16F)
│   ├─ RT2: WorldPosition.rgb + AO.a       (RGBA16F)
│   └─ Depth: Depth24_Stencil8
│
├─ Lighting Pass（全屏四边形）:
│   ├─ 输入: GBuffer 纹理 + 光源 UBO
│   ├─ 计算: 逐像素 PBR 光照（Cook-Torrance）
│   └─ 输出: 最终颜色到交换链图像
│
└─ (可选) 后处理 Pass: Tone Mapping / Gamma 校正
```

#### 3.6.4 PBR 材质

```cpp
// Renderer/Public/Material.h
#pragma once
#include "Math/MathTypes.h"
#include <memory>

namespace TE {

class IRHITexture;

struct PBRMaterialParams
{
    Vector4 BaseColor       = Vector4(1.0f);   // 基础颜色（无纹理时使用）
    float   Metallic        = 0.0f;            // 金属度 0~1
    float   Roughness       = 0.5f;            // 粗糙度 0~1
    float   AO              = 1.0f;            // 环境遮蔽
    float   NormalStrength  = 1.0f;            // 法线强度
};

class Material
{
public:
    PBRMaterialParams Params;

    // PBR 纹理贴图（nullptr 表示使用 Params 中的常量值）
    std::shared_ptr<IRHITexture> AlbedoMap;      // 反照率贴图
    std::shared_ptr<IRHITexture> NormalMap;       // 法线贴图
    std::shared_ptr<IRHITexture> MetallicMap;     // 金属度贴图
    std::shared_ptr<IRHITexture> RoughnessMap;    // 粗糙度贴图
    std::shared_ptr<IRHITexture> AOMap;           // 环境遮蔽贴图
};

} // namespace TE
```

#### 3.6.5 灯光

```cpp
// Renderer/Public/Light.h
#pragma once
#include "Math/MathTypes.h"

namespace TE {

enum class ELightType { Directional, Point, Spot };

struct LightData
{
    ELightType Type       = ELightType::Directional;
    Vector3    Position   = Vector3(0.0f);         // Point/Spot 使用
    Vector3    Direction  = Vector3(0.0f, -1.0f, 0.0f); // Directional/Spot 使用
    Vector3    Color      = Vector3(1.0f);
    float      Intensity  = 1.0f;
    float      Range      = 10.0f;                 // Point/Spot 的衰减范围
    float      SpotAngle  = 45.0f;                 // Spot 内锥角（度）
};

} // namespace TE
```

---

### 3.7 Scene — 场景系统 [必须]

> 简化版 UE5 的 World / Actor / Component 体系。

#### 3.7.1 整体结构

```
TWorld（场景容器）
 │
 ├── TActor "Sun"
 │    └── Components:
 │         └── TLightComponent (Directional Light)
 │
 ├── TActor "MainCamera"
 │    └── Components:
 │         └── TCameraComponent (透视投影)
 │
 └── TActor "Helmet"
      ├── RootComponent: TSceneComponent (Position, Rotation, Scale)
      └── Components:
           └── TMeshComponent (引用 Mesh + Material)
```

#### 3.7.2 TActor

```cpp
// Scene/Public/Actor.h
#pragma once
#include "Object.h"
#include "ObjectMacros.h"
#include <vector>
#include <memory>

namespace TE {

class TComponent;
class TSceneComponent;
class TWorld;

class TActor : public TObject
{
    TE_DECLARE_CLASS(TActor, TObject)

public:
    TActor();
    virtual ~TActor();

    // 组件管理
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = comp.get();
        comp->m_Owner = this;
        m_Components.push_back(std::move(comp));
        return ptr;
    }

    template<typename T>
    T* GetComponent() const
    {
        for (auto& comp : m_Components)
        {
            if (auto* casted = dynamic_cast<T*>(comp.get()))
                return casted;
        }
        return nullptr;
    }

    // 生命周期
    virtual void BeginPlay();
    virtual void Tick(float DeltaTime);

    // 变换（委托给 RootComponent）
    TSceneComponent* GetRootComponent() const { return m_RootComponent; }
    void SetRootComponent(TSceneComponent* root) { m_RootComponent = root; }

    TWorld* GetWorld() const { return m_World; }

private:
    friend class TWorld;
    TWorld* m_World = nullptr;
    TSceneComponent* m_RootComponent = nullptr;
    std::vector<std::unique_ptr<TComponent>> m_Components;
};

} // namespace TE
```

#### 3.7.3 TComponent 体系

```cpp
// Scene/Public/Component.h
#pragma once
#include "Object.h"
#include "ObjectMacros.h"

namespace TE {

class TActor;

class TComponent : public TObject
{
    TE_DECLARE_CLASS(TComponent, TObject)

public:
    virtual ~TComponent() = default;

    TActor* GetOwner() const { return m_Owner; }

    virtual void BeginPlay() {}
    virtual void Tick(float DeltaTime) {}

private:
    friend class TActor;
    TActor* m_Owner = nullptr;
};

} // namespace TE
```

```cpp
// Scene/Public/SceneComponent.h — 带 Transform 的组件
#pragma once
#include "Component.h"
#include "Math/MathTypes.h"
#include <vector>

namespace TE {

class TSceneComponent : public TComponent
{
    TE_DECLARE_CLASS(TSceneComponent, TComponent)

public:
    Transform& GetTransform() { return m_LocalTransform; }
    const Transform& GetTransform() const { return m_LocalTransform; }

    Matrix4 GetWorldMatrix() const;

    // 父子层级（用于骨骼、挂接点等）
    void AttachTo(TSceneComponent* parent);
    const std::vector<TSceneComponent*>& GetChildren() const { return m_Children; }

private:
    Transform m_LocalTransform;
    TSceneComponent* m_Parent = nullptr;
    std::vector<TSceneComponent*> m_Children;
};

} // namespace TE
```

```cpp
// Scene/Public/MeshComponent.h — 可渲染网格组件
#pragma once
#include "SceneComponent.h"
#include <memory>

namespace TE {

class Mesh;
class Material;

class TMeshComponent : public TSceneComponent
{
    TE_DECLARE_CLASS(TMeshComponent, TSceneComponent)

public:
    void SetMesh(std::shared_ptr<Mesh> mesh) { m_Mesh = mesh; }
    void SetMaterial(std::shared_ptr<Material> material) { m_Material = material; }

    Mesh*     GetMesh() const { return m_Mesh.get(); }
    Material* GetMaterial() const { return m_Material.get(); }

private:
    std::shared_ptr<Mesh>     m_Mesh;
    std::shared_ptr<Material> m_Material;
};

} // namespace TE
```

```cpp
// Scene/Public/LightComponent.h
#pragma once
#include "SceneComponent.h"
#include "Light.h"

namespace TE {

class TLightComponent : public TSceneComponent
{
    TE_DECLARE_CLASS(TLightComponent, TSceneComponent)

public:
    LightData& GetLightData() { return m_LightData; }

private:
    LightData m_LightData;
};

} // namespace TE
```

```cpp
// Scene/Public/CameraComponent.h
#pragma once
#include "SceneComponent.h"
#include "Math/MathTypes.h"

namespace TE {

class TCameraComponent : public TSceneComponent
{
    TE_DECLARE_CLASS(TCameraComponent, TSceneComponent)

public:
    float FOV         = 60.0f;   // 垂直视角（度）
    float NearPlane   = 0.1f;
    float FarPlane    = 1000.0f;
    float AspectRatio = 16.0f / 9.0f;

    Matrix4 GetViewMatrix() const;
    Matrix4 GetProjectionMatrix() const;
};

} // namespace TE
```

#### 3.7.4 TWorld

```cpp
// Scene/Public/World.h
#pragma once
#include "Object.h"
#include <vector>
#include <memory>

namespace TE {

class TActor;

class TWorld : public TObject
{
    TE_DECLARE_CLASS(TWorld, TObject)

public:
    // 创建并加入 Actor
    template<typename T = TActor, typename... Args>
    T* SpawnActor(Args&&... args)
    {
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = actor.get();
        ptr->m_World = this;
        m_Actors.push_back(std::move(actor));
        ptr->BeginPlay();
        return ptr;
    }

    void Tick(float DeltaTime);

    const std::vector<std::unique_ptr<TActor>>& GetActors() const { return m_Actors; }

private:
    std::vector<std::unique_ptr<TActor>> m_Actors;
};

} // namespace TE
```

---

### 3.8 Asset — 资源系统 [必须]

> 简化版资源管理，提供模型和纹理的加载、缓存。

#### 3.8.1 Mesh 数据结构

```cpp
// Asset/Public/Mesh.h
#pragma once
#include "Math/MathTypes.h"
#include <vector>
#include <cstdint>

namespace TE {

struct Vertex
{
    Vector3 Position;
    Vector3 Normal;
    Vector2 TexCoord;
    Vector3 Tangent;   // 用于法线贴图
};

class Mesh
{
public:
    std::vector<Vertex>   Vertices;
    std::vector<uint32_t> Indices;

    // GPU 侧资源句柄（由 Renderer 在上传后填入）
    class IRHIBuffer* VertexBuffer = nullptr;
    class IRHIBuffer* IndexBuffer  = nullptr;
};

} // namespace TE
```

#### 3.8.2 Texture 数据结构

```cpp
// Asset/Public/Texture.h
#pragma once
#include <cstdint>
#include <vector>
#include <memory>

namespace TE {

class IRHITexture;

class Texture
{
public:
    uint32_t Width  = 0;
    uint32_t Height = 0;
    uint32_t Channels = 4;
    std::vector<uint8_t> Pixels;   // CPU 侧像素数据

    // GPU 侧资源句柄
    std::shared_ptr<IRHITexture> RHITexture;
};

} // namespace TE
```

#### 3.8.3 AssetManager

```cpp
// Asset/Public/AssetManager.h
#pragma once
#include <string>
#include <memory>
#include <unordered_map>

namespace TE {

class Mesh;
class Texture;

class AssetManager
{
public:
    // 加载并缓存（重复路径直接返回缓存）
    std::shared_ptr<Mesh>    LoadMesh(const std::string& path);
    std::shared_ptr<Texture> LoadTexture(const std::string& path);

    void Clear();

private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>>    m_MeshCache;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_TextureCache;
};

} // namespace TE
```

**模型加载器支持：**
- OBJ 格式 — 使用 tinyobjloader [已有]
- glTF 2.0 格式 — 使用 cgltf 或 tinygltf [推荐新增]

**纹理加载器支持：**
- PNG / JPG / BMP / TGA — 使用 stb_image [已有]

---

### 3.9 Input — 输入系统 [可选]

> 简化版输入管理，封装键盘/鼠标状态查询。当前可直接使用 Window 的回调，
> 后续需要更复杂的输入映射时再实现此模块。

```cpp
// Input/Public/InputSystem.h（可选实现）
#pragma once

namespace TE {

class InputSystem
{
public:
    static InputSystem& Get();

    bool IsKeyDown(int keyCode) const;
    bool IsKeyPressed(int keyCode) const;  // 本帧按下

    bool IsMouseButtonDown(int button) const;
    float GetMouseX() const;
    float GetMouseY() const;
    float GetMouseDeltaX() const;
    float GetMouseDeltaY() const;

    void Update();  // 每帧开始时调用，更新状态
};

} // namespace TE
```

---

## 4. 第三方库清单

### 4.1 已有（继续使用）

| 库 | 用途 | 类型 |
|---|---|---|
| glfw-3.4 | 窗口管理、OpenGL context 创建 | 静态库 |
| glm | 数学库（向量、矩阵、四元数） | 头文件库 |
| spdlog | 日志 | 头文件库 |
| tlsf | CPU 内存分配器 | OBJECT 库 |
| stb-master | 图像加载（stb_image） | 单头文件 |
| tinyobjloader | OBJ 模型加载 | 单头文件 |

### 4.2 按图形后端需要新增

选择哪个后端，就引入对应的库：

**Vulkan 后端：**

| 库 | 用途 | 获取方式 |
|---|---|---|
| Vulkan SDK | 运行时 + 验证层 + glslc 编译器 | 系统安装（不放入 ThirdParty） |
| vulkan-hpp | Vulkan C++ 绑定 + RAII 封装 | Vulkan SDK 附带 |
| VMA (Vulkan Memory Allocator) | GPU 显存管理 | 单头文件，放入 ThirdParty |
| shaderc 或 glslc | GLSL → SPIR-V 编译 | Vulkan SDK 附带 |

**OpenGL 后端：**

| 库 | 用途 | 获取方式 |
|---|---|---|
| glad 或 glew | OpenGL 函数加载器 | 放入 ThirdParty |
| (GLFW 已有) | 创建 OpenGL context | 已有 |

**DirectX 12 后端（仅 Windows）：**

| 库 | 用途 | 获取方式 |
|---|---|---|
| D3D12 SDK | D3D12 运行时 | Windows SDK 自带 |
| DXC | HLSL 着色器编译 | Windows SDK 或单独下载 |
| D3D12MA (可选) | GPU 显存管理 | 单头文件 |

### 4.3 推荐新增（与后端无关）

| 库 | 用途 | 必要性 |
|---|---|---|
| cgltf 或 tinygltf | glTF 2.0 模型加载（自带 PBR 材质） | 推荐 |
| Dear ImGui | 调试 UI / 编辑器界面 | 可选 |
| Assimp | 更多模型格式（FBX 等） | 可选 |
| Tracy | 性能分析 | 可选 |
| EnTT | ECS 框架（替代自写 Component） | 可选 |

---

## 5. Shader 跨后端策略

### 5.1 着色器源语言

以 **GLSL** 作为主要着色器编写语言（UE5 使用 HLSL，但 GLSL 对 Vulkan 和 OpenGL 都友好）。

### 5.2 各后端处理方式

| 后端 | Shader 输入格式 | 编译/转换方式 |
|---|---|---|
| Vulkan | SPIR-V 字节码 | 离线用 `glslc` 编译 GLSL → SPIR-V |
| OpenGL | GLSL 源码 | 直接加载 GLSL 源文件，运行时编译 |
| D3D12 | HLSL 或 DXIL | 用 SPIRV-Cross 将 SPIR-V 转译为 HLSL，或直接编写 HLSL |

### 5.3 Shader 目录约定

```
Content/Shaders/
├── Common/
│   ├── PBRLighting.glsl       # PBR Cook-Torrance BRDF 函数
│   ├── ToneMapping.glsl       # 色调映射
│   └── Constants.glsl         # 公共常量
├── Forward/
│   ├── ForwardPBR.vert        # 前向 PBR 顶点着色器
│   └── ForwardPBR.frag        # 前向 PBR 片段着色器
├── Deferred/
│   ├── GBuffer.vert           # GBuffer 顶点着色器
│   ├── GBuffer.frag           # GBuffer 片段着色器
│   ├── DeferredLighting.vert  # 全屏四边形顶点着色器
│   └── DeferredLighting.frag  # 延迟光照片段着色器
└── PostProcess/
    └── FinalComposite.frag    # Tone Mapping + Gamma 校正
```

### 5.4 CMake 着色器编译（Vulkan 后端示例）

```cmake
# CMake/CompileShaders.cmake
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)

function(compile_glsl_to_spirv TARGET SHADER_SOURCE OUTPUT_DIR)
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
    set(SPIRV_OUTPUT "${OUTPUT_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${GLSLC} ${SHADER_SOURCE} -o ${SPIRV_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling ${SHADER_NAME} -> SPIR-V"
    )

    target_sources(${TARGET} PRIVATE ${SPIRV_OUTPUT})
endfunction()
```

---

## 6. 技术实施路线

### Phase 1: 基础设施修复 + 引擎骨架 [必须]

**目标：** 建立引擎主循环，能打开窗口并稳定运行空场景。

- [ ] 修复已知 Bug（Memory.cpp 递归调用、Main.cpp 窗口创建）
- [ ] 修复 CMake 配置（ThirdParty 头文件泄漏）
- [ ] 实现 `Core/Math/MathTypes.h`（glm 类型封装 + Transform）
- [ ] 实现 `Engine` 模块（Init / Run / Shutdown / Tick）
- [ ] 重构 `Sandbox/Main.cpp` 改为调用 `Engine::Get().Init(); Engine::Get().Run();`
- [ ] 创建 `CMake/EngineOptions.cmake`（TE_RHI_VULKAN / TE_RHI_OPENGL / TE_RHI_D3D12 选项）

**验收标准：** Sandbox 启动后打开窗口，稳定运行主循环，按关闭按钮正常退出。

---

### Phase 2: RHI 抽象层 + 首个图形后端 [必须]

**目标：** 实现 RHI 接口并完成一个图形后端，渲染一个彩色三角形。

- [ ] 定义 RHI 纯虚接口（IRHIDevice / IRHIBuffer / IRHITexture / IRHIPipeline 等）
- [ ] 实现 `RHIFactory`（根据 CMake 选项和配置文件选择后端）
- [ ] **选择一个图形 API 作为首个后端实现**
  - 实现 Device / SwapChain / CommandBuffer
  - 实现 Buffer / Texture / Shader
  - 实现 Pipeline / RenderPass / Descriptor
- [ ] 在 Sandbox 中渲染一个硬编码的彩色三角形

**验收标准：** 窗口中显示一个彩色三角形，验证 RHI 接口可行。

> **建议：** 完成首个后端后，审视 RHI 接口设计是否足够通用。如果发现有过度耦合特定 API 的设计，趁早调整。

---

### Phase 3: 资源系统 + 基础渲染 [必须]

**目标：** 加载真实模型和纹理，实现前向 PBR 渲染。

- [ ] 实现 Mesh / Texture 数据结构
- [ ] 实现 AssetManager + MeshLoader（tinyobjloader）
- [ ] 实现 TextureLoader（stb_image）
- [ ] 实现 Material（PBR 参数 + 纹理绑定）
- [ ] 编写 Forward PBR Shader（Cook-Torrance BRDF）
- [ ] 实现 ForwardRenderer（遍历可渲染对象 → 绑定管线 → Draw）
- [ ] 实现简单相机控制（WASD + 鼠标旋转）

**验收标准：** 窗口中显示一个 PBR 渲染的 3D 模型（如 DamagedHelmet），支持方向光照明。

---

### Phase 4: 对象系统 + 场景系统 [必须]

**目标：** 建立 Actor-Component 体系，用场景管理取代硬编码渲染。

- [ ] 实现 TObject / TClass / 类型注册宏
- [ ] 实现 TActor + TComponent + TSceneComponent
- [ ] 实现 TMeshComponent / TLightComponent / TCameraComponent
- [ ] 实现 TWorld（SpawnActor / Tick / Actor 遍历）
- [ ] 重构 Renderer：从 World 收集 MeshComponent → 构建 RenderScene → 渲染
- [ ] 重构 Sandbox：通过 SpawnActor + AddComponent 搭建场景

**验收标准：** 通过 `world->SpawnActor()` + `actor->AddComponent<TMeshComponent>()` 方式搭建场景，渲染效果与 Phase 3 一致。

---

### Phase 5: 延迟渲染 [推荐]

**目标：** 实现延迟渲染管线，支持前向/延迟切换。

- [ ] 实现 GBuffer RenderPass（多 RenderTarget 输出）
- [ ] 实现 Lighting Pass（全屏四边形 + 逐像素 PBR 光照）
- [ ] 支持多光源（点光、方向光、聚光灯）
- [ ] 实现 SceneRenderer 的 Forward / Deferred 模式切换
- [ ] (可选) 简单后处理：Tone Mapping + Gamma 校正

**验收标准：** 可在运行时切换前向/延迟渲染，效果基本一致，延迟渲染下支持更多光源。

---

### Phase 6: 第二图形后端 [可选]

**目标：** 验证 RHI 抽象的解耦性。

- [ ] 选择第二个图形 API 实现后端
- [ ] 确保 Renderer 层代码零修改即可切换后端

**验收标准：** 修改配置文件或 CMake 选项即可切换后端，相同场景正常渲染。

---

### Phase 7: 可选扩展

以下功能可按兴趣选择实现：

| 功能 | 说明 | 难度 |
|---|---|---|
| glTF 加载 | PBR 材质直接读取，比 OBJ 更完整 | 中 |
| Dear ImGui 集成 | 运行时调试面板（材质编辑、场景查看） | 低 |
| 天空盒 | Cubemap 天空背景 + 环境反射 | 低 |
| 阴影映射 | 方向光 Shadow Map（基础） | 中 |
| IBL 环境光照 | HDRI → Irradiance + Prefilter + BRDF LUT | 高 |
| 简单地形 | 高度图生成网格 + 多纹理混合 | 中 |
| 输入系统 | 键盘/鼠标状态管理、Action 映射 | 低 |
| 动画系统 | 骨骼动画（需扩展 Mesh/Component） | 高 |
| 物理系统 | 集成 Bullet/Jolt（碰撞检测 + 刚体） | 高 |
| 2D 渲染 | Sprite 组件 + 正交投影 | 中 |

---

## 7. 关键架构决策

### 7.1 图形 API 策略

不绑定任何具体 API。RHI 为纯抽象接口，后端可插拔（Vulkan / OpenGL / D3D12）。建议先选一个实现，验证接口设计后再扩展。

### 7.2 GPU 内存管理

由各后端自行处理：
- Vulkan → VMA (Vulkan Memory Allocator)
- OpenGL → 原生 Buffer/Texture 管理
- D3D12 → D3D12MA 或手动管理

### 7.3 对象生命周期

使用 `std::shared_ptr` + `std::unique_ptr` 管理，不实现 UE5 的完整 GC。World 持有 Actor 的 `unique_ptr`，Actor 持有 Component 的 `unique_ptr`，资源（Mesh/Texture/Material）使用 `shared_ptr` 支持多处引用。

### 7.4 组件模式

采用 Actor-Component 模式（非纯 ECS），更接近 UE5，概念清晰易于理解：
- `TActor` = 场景中的实体
- `TComponent` = 功能单元（渲染、灯光、相机等）
- `TSceneComponent` = 带 Transform 的组件，支持父子层级

### 7.5 PBR 渲染模型

Cook-Torrance 微表面 BRDF，这是业界标准：
- 法线分布函数 (D): GGX/Trowbridge-Reitz
- 几何遮蔽函数 (G): Smith's Schlick-GGX
- 菲涅尔项 (F): Schlick 近似

### 7.6 Shader 策略

以 GLSL 为主要编写语言。Vulkan 后端离线编译为 SPIR-V，OpenGL 后端直接使用 GLSL，D3D12 后端可通过 SPIRV-Cross 转译或直接编写 HLSL。

---

## 8. 当前代码问题与改进建议

### 8.1 已修复（工作区已改）

| 问题 | 文件 | 说明 |
|---|---|---|
| MemRealloc 递归调用 | `Core/Private/Memory/Memory.cpp` | 原来调用自身，已改为调用 `MemAlignedRealloc` |
| Window 创建错误 | `Sandbox/Main.cpp` | 原来误用 `MemAlloc(config)`，已改为 `Window::Create(config)` |

### 8.2 待改进

| 问题 | 文件 | 建议 |
|---|---|---|
| ThirdParty 头文件全局泄漏 | `Core/CMakeLists.txt` | `target_include_directories(Core PUBLIC ThirdParty)` 把所有第三方头暴露给消费者。短期可接受（因为 Log.h 公开头文件包含了 spdlog），长期建议将 spdlog 包裹在 Core 内部接口后面，改为 PRIVATE |
| 缺少引擎主循环 | 无 Engine 模块 | Phase 1 中实现 |
| git 残留已删除文件 | 多个模块 | 建议 `git clean` 或重新搭建目录骨架 |
| Log.h 公开暴露 spdlog | `Core/Public/Log/Log.h` | 日志宏展开后依赖 spdlog 类型。可通过 pimpl 或仅在宏中使用不透明类型来隐藏（优先级低，学习项目可接受） |
| C++ 标准建议升级 | `CMakeLists.txt` | 当前 C++17，建议升级到 C++20（if constexpr、concepts 等对模板代码更友好，Vulkan-Hpp RAII 也更好用） |

---

## 9. 参考资料

### 引擎架构
- UE5 源码（`Engine/Source/Runtime/` 目录结构）
- [Game Engine Architecture - Jason Gregory](https://www.gameenginebook.com/)
- [Hazel Engine (TheCherno)](https://github.com/TheCherno/Hazel) — 类似的学习引擎项目

### PBR 渲染
- [Learn OpenGL - PBR Theory](https://learnopengl.com/PBR/Theory)
- [Real Shading in Unreal Engine 4 - Brian Karis](https://blog.selfshadow.com/publications/s2013-shading-course/)
- [glTF 2.0 PBR Specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material)

### Vulkan
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [vkguide.dev](https://vkguide.dev/) — 现代 Vulkan 引擎教程
- [Vulkan-Hpp Samples](https://github.com/KhronosGroup/Vulkan-Hpp)

### OpenGL
- [Learn OpenGL](https://learnopengl.com/)

### 延迟渲染
- [Learn OpenGL - Deferred Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)
- [Filament Rendering Architecture](https://google.github.io/filament/Filament.html)
