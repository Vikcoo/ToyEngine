# Asset 模块

资产管理系统，负责资源的加载、缓存和管理。

## 📁 目录结构

- **Public/**
  - `Asset.h` - 资产基类
  - `AssetManager.h` - 资产管理器（单例）
  - `AssetLoader.h` - 加载器接口

- **Private/**
  - `AssetManager.cpp`
  - **Loaders/** - 各种资源加载器
    - `TextureLoader.cpp` - 纹理加载（基于stb_image）
    - `ModelLoader.cpp` - 模型加载（基于tinyobjloader）
    - `ShaderLoader.cpp` - Shader加载

## 🎯 设计目标

1. **自动缓存** - 同一资源只加载一次
2. **延迟加载** - 按需加载资源
3. **类型安全** - 模板保证类型正确性

## 📝 使用示例

```cpp
#include "Asset/Public/AssetManager.h"

using namespace TE;

// 加载纹理（自动缓存）
auto texture = AssetManager::Load<Texture2D>("textures/wall.png");

// 加载模型
auto mesh = AssetManager::Load<Mesh>("models/cube.obj");

// 重复加载会返回缓存
auto texture2 = AssetManager::Load<Texture2D>("textures/wall.png");
// texture == texture2

// 清理所有资源
AssetManager::Clear();
```

## 🔗 依赖

- stb_image (图像加载)
- tinyobjloader (模型加载)
- RHI模块
- Core模块

## ✅ 实现清单

- [ ] Asset基类
- [ ] AssetManager
- [ ] TextureLoader（PNG, JPG等）
- [ ] ModelLoader（OBJ格式）
- [ ] ShaderLoader（SPIR-V）
- [ ] 资产引用计数
- [ ] 资产卸载机制

