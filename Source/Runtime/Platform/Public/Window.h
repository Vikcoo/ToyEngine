// ToyEngine Platform Module
// Window抽象接口 - 跨平台窗口抽象

#pragma once
#include <string>
#include <memory>
#include <functional>
#include <cstdint>

namespace TE {

// 窗口配置
struct WindowConfig {
    std::string title = "ToyEngine";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool resizable = true;
    bool vsync = true;
};

// 窗口事件回调类型定义
using WindowResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
using WindowCloseCallback = std::function<void()>;
using WindowFocusCallback = std::function<void(bool focused)>;

// 窗口抽象接口
class Window {
public:
    virtual ~Window() = default;

    // 窗口生命周期
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void PollEvents() = 0;
    [[nodiscard]] virtual bool ShouldClose() const = 0;

    // 窗口属性
    [[nodiscard]] virtual uint32_t GetWidth() const = 0;
    [[nodiscard]] virtual uint32_t GetHeight() const = 0;
    [[nodiscard]] virtual const std::string& GetTitle() const = 0;

    // 原生句柄（用于创建Vulkan Surface等）
    [[nodiscard]] virtual void* GetNativeHandle() const = 0;

    // 窗口事件回调设置
    virtual void SetResizeCallback(WindowResizeCallback callback) = 0;
    virtual void SetCloseCallback(WindowCloseCallback callback) = 0;
    virtual void SetFocusCallback(WindowFocusCallback callback) = 0;

    // 工厂方法 - 根据平台自动创建合适的窗口实现
    static std::unique_ptr<Window> Create(const WindowConfig& config = WindowConfig());

protected:
    Window() = default;
};

} // namespace TE


