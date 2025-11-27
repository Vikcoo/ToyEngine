# Platform 模块

平台抽象层，提供跨平台的操作系统功能。

## 📁 目录结构

- **Public/** - 平台抽象接口
  - `Window.h` - 窗口抽象接口
  - `Input.h` - 输入抽象接口
  - `PlatformUtils.h` - 平台工具函数

- **Private/** - 平台具体实现
  - **GLFW/** - 基于GLFW的跨平台实现
    - `GLFWWindow.h/cpp` - GLFW窗口实现
    - `GLFWInput.h/cpp` - GLFW输入实现
  - `PlatformFactory.cpp` - 平台工厂

## 🎯 设计目标

1. **抽象窗口操作** - 创建、销毁、事件处理
2. **抽象输入处理** - 键盘、鼠标输入
3. **跨平台支持** - Windows、Linux、macOS

## 📝 接口示例

```cpp
// Window抽象接口
class Window {
public:
    virtual ~Window() = default;
    
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual bool ShouldClose() const = 0;
    virtual void PollEvents() = 0;
    virtual void* GetNativeHandle() const = 0;
    
    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    
    static std::unique_ptr<Window> Create(uint32_t width, uint32_t height, const std::string& title);
};
```

## 🔗 依赖

- GLFW (窗口和输入)
- Core模块

## ✅ 实现清单

- [ ] Window抽象接口
- [ ] GLFW窗口实现
- [ ] Input抽象接口
- [ ] GLFW输入实现
- [ ] 窗口事件系统

