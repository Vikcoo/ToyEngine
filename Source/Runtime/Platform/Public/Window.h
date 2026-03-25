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
};

// 窗口事件回调类型定义
using WindowResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;

// 窗口抽象接口
class Window {
public:
    virtual ~Window() = default;

    // 窗口生命周期
    virtual void PollEvents() = 0;
    [[nodiscard]] virtual bool ShouldClose() const = 0;

    // 窗口逻辑尺寸（屏幕坐标）
    [[nodiscard]] virtual uint32_t GetWidth() const = 0;
    [[nodiscard]] virtual uint32_t GetHeight() const = 0;

    // 帧缓冲区实际像素尺寸（Retina 等高 DPI 屏幕上可能是逻辑尺寸的 2 倍）
    // glViewport / 渲染分辨率应使用此尺寸
    [[nodiscard]] virtual uint32_t GetFramebufferWidth() const = 0;
    [[nodiscard]] virtual uint32_t GetFramebufferHeight() const = 0;

    // 原生句柄（用于后续扩展）
    [[nodiscard]] virtual void* GetNativeHandle() const = 0;

    // 窗口事件回调设置
    virtual void SetResizeCallback(WindowResizeCallback callback) = 0;
    virtual void SetKeyCallback(KeyCallback callback) = 0;

    // OpenGL 双缓冲交换
    virtual void SwapBuffers() = 0;

    /// 设置垂直同步
    /// @param enabled true=锁定到显示器刷新率, false=不限帧
    virtual void SetVSync(bool enabled) = 0;

    /// 查询当前 VSync 状态
    [[nodiscard]] virtual bool IsVSyncEnabled() const = 0;

    // 工厂方法 - 根据平台自动创建合适的窗口实现
    [[nodiscard]] static std::unique_ptr<Window> Create(const WindowConfig& config = WindowConfig());

protected:
    Window() = default;
};

} // namespace TE
