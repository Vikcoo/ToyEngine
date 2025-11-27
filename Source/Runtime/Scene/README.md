# Scene 模块

场景管理系统，提供场景图和实体组件系统。

## 📁 目录结构

- **Public/**
  - `Scene.h` - 场景类
  - `Entity.h` - 实体类
  - **Components/** - 组件
    - `Component.h` - 组件基类
    - `TransformComponent.h` - 变换组件
    - `MeshComponent.h` - 网格组件
    - `CameraComponent.h` - 相机组件
    - `LightComponent.h` - 光源组件
  - `SceneSerializer.h` - 场景序列化（可选）

- **Private/**
  - `Scene.cpp`
  - `Entity.cpp`

## 🎯 设计选择

有两个设计方向：

### 选项A：使用ECS库（推荐）
- 使用EnTT库
- 高性能
- 数据导向设计

### 选项B：简单对象树
- 传统继承结构
- 易于理解
- 适合小型项目

## 📝 使用示例（ECS方式）

```cpp
#include "Scene/Public/Scene.h"

using namespace TE;

// 创建场景
Scene scene;

// 创建实体
Entity entity = scene.CreateEntity("MyObject");

// 添加组件
auto& transform = entity.AddComponent<TransformComponent>();
transform.position = Vector3(0, 0, 0);

auto& meshComp = entity.AddComponent<MeshComponent>();
meshComp.mesh = mesh;
meshComp.material = material;

// 更新场景
scene.Update(deltaTime);

// 渲染场景
scene.Render();
```

## 🔗 依赖

- EnTT (ECS库，可选)
- Renderer模块
- Core模块

## ✅ 实现清单

- [ ] Scene类
- [ ] Entity类
- [ ] TransformComponent
- [ ] MeshComponent
- [ ] CameraComponent
- [ ] LightComponent
- [ ] 场景更新系统
- [ ] 场景序列化（可选）

