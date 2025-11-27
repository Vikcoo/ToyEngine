# ToyEngine

一个轻量级、跨平台、支持多图形API的游戏引擎。

## 🎯 项目特性

- ✅ **跨平台支持** - Windows, Linux, macOS
- ✅ **多图形API** - Vulkan, DirectX 12, OpenGL
- ✅ **模块化设计** - 清晰的模块边界和依赖关系
- ✅ **现代C++** - C++17标准
- ✅ **易于扩展** - 插件化架构设计

## 📁 目录结构

```
ToyEngine/
├── CMake/              # CMake辅助脚本
├── Content/            # 引擎资源
│   ├── Shaders/       # Shader源码
│   ├── Textures/      # 纹理资源
│   ├── Models/        # 模型资源
│   └── Config/        # 配置文件
├── Docs/              # 文档
├── Source/            # 源代码
│   ├── Runtime/       # 运行时引擎核心
│   │   ├── Core/     # 核心基础设施
│   │   ├── Platform/ # 平台抽象层
│   │   ├── RHI/      # 渲染硬件接口
│   │   ├── Renderer/ # 高层渲染器
│   │   ├── Asset/    # 资产管理
│   │   ├── Scene/    # 场景系统
│   │   └── Input/    # 输入系统
│   └── Sandbox/       # 沙盒/示例程序
└── ThirdParty/        # 第三方库
```

## 🚀 构建说明

### 前置要求

- CMake 3.20+
- C++17 编译器
- Vulkan SDK (如果使用Vulkan后端)

### 构建步骤

```bash
# 1. 克隆仓库
git clone https://github.com/yourusername/ToyEngine.git
cd ToyEngine

# 2. 创建构建目录
mkdir build
cd build

# 3. 生成项目文件
cmake ..

# 4. 编译
cmake --build .
```

## 📚 架构设计

### RHI层（渲染硬件接口）

RHI层提供了统一的渲染接口抽象，支持多种图形API：

- **Public/** - 纯抽象接口，零图形API依赖
- **Private/** - 具体图形API实现（Vulkan, D3D12等）

上层代码（Renderer、Application）完全不知道底层图形API的存在。

### 模块依赖关系

```
Application/Game
       ↓
   Renderer          Scene
       ↓               ↓
      RHI    ←    Asset
       ↓
   Platform    ←    Input
       ↓
     Core
```

## 🛠️ 开发路线

- [x] 目录结构设计
- [ ] Core模块（日志、数学、文件系统）
- [ ] Platform模块（窗口、输入）
- [ ] RHI模块（Vulkan实现）
- [ ] Renderer模块（基础渲染器）
- [ ] Asset模块（资产管理）
- [ ] Scene模块（场景系统）

## 📖 文档

详细文档请查看 `Docs/` 目录：

- [架构设计](Docs/Architecture.md)
- [API参考](Docs/API/)

## 📄 许可证

MIT License

## 🤝 贡献

欢迎贡献！请查看贡献指南。

---

**作者**: YuKai  
**开始日期**: 2025-08-23

