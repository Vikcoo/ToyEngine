# ToyEngine

一个目标是轻量级、跨平台、支持多图形API的游戏引擎。

## 目录结构

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

##  构建说明

- CMake 3.20+
- C++17 编译器
- Vulkan SDK

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

##  许可证
MIT License

---

**作者**: YuKai  
**开始日期**: 2025-07-23

