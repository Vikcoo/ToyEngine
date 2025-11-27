# Input 模块

输入系统，提供键盘、鼠标等输入设备的抽象。

## 📁 目录结构

- **Public/**
  - `Input.h` - 输入管理器（静态接口）
  - `KeyCodes.h` - 键码定义

- **Private/**
  - `Input.cpp` - 输入实现

## 📝 使用示例

```cpp
#include "Input/Public/Input.h"

using namespace TE;

// 检查键盘输入
if (Input::IsKeyPressed(KeyCode::W)) {
    camera.MoveForward(speed * deltaTime);
}

// 检查鼠标按钮
if (Input::IsMouseButtonPressed(MouseButton::Left)) {
    // 处理点击
}

// 获取鼠标位置
auto [x, y] = Input::GetMousePosition();

// 获取鼠标滚轮
float scroll = Input::GetMouseScrollDelta();
```

## 🔗 依赖

- Platform模块
- Core模块

## ✅ 实现清单

- [ ] Input管理器
- [ ] 键码定义
- [ ] 键盘输入
- [ ] 鼠标输入
- [ ] 滚轮输入
- [ ] 输入事件系统

