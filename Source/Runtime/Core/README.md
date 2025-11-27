# Core 模块

核心基础设施模块，提供引擎的基础功能。

## 📁 目录结构

- **Base/** - 基础类型定义、宏定义
  - `Types.h` - 引擎类型定义（int32, uint64等）
  - `NonCopyable.h` - 禁止拷贝基类
  - `Singleton.h` - 单例模板

- **Math/** - 数学库封装（基于glm）
  - `MathTypes.h` - Vector3, Matrix4等类型定义
  - `Transform.h` - 变换类（位置、旋转、缩放）
  - `MathUtils.h` - 常用数学函数

- **Memory/** - 内存管理
  - `RefCounted.h` - 引用计数基类
  - `Allocator.h` - 自定义分配器（可选）

- **Container/** - 容器封装（可选）
  - STL容器的简单封装

- **String/** - 字符串工具
  - `StringUtils.h` - 字符串操作函数

- **FileSystem/** - 文件系统
  - `Path.h` - 路径操作
  - `FileUtils.h` - 文件读写工具

- **Log/** - 日志系统
  - `Log.h` - 日志接口（基于spdlog）

- **Application/** - 应用程序框架
  - `Application.h` - 应用程序基类
  - `Timer.h` - 时间工具

## 🔗 依赖

- glm (数学库)
- spdlog (日志库)

## 📝 使用示例

```cpp
#include "Core/Public/Log/Log.h"
#include "Core/Public/Math/MathTypes.h"

using namespace TE;

int main() {
    Log::Init();
    
    Vector3 position(0.0f, 1.0f, 0.0f);
    TE_LOG_INFO("Position: ({}, {}, {})", position.x, position.y, position.z);
    
    return 0;
}
```

## ✅ 实现清单

- [ ] Base类型定义
- [ ] 日志系统
- [ ] 数学库封装
- [ ] 文件系统工具
- [ ] Application框架
- [ ] 时间管理

