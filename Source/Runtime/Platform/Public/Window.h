// ToyEngine Platform Module
// Window抽象接口 - 跨平台窗口抽象

#pragma once
#include <string>
#include <memory>
#include <functional>
#include <cstdint>

namespace TE {

/// 窗口创建时使用的图形 API 类型
/// 决定窗口是否创建 OpenGL Context 还是以 No-API 模式运行（供 Vulkan/D3D12 使用）
enum class EWindowGraphicsAPI : uint8_t
{
    OpenGL,     // 创建 OpenGL Context + 加载 glad
    None,       // 不创建任何图形上下文（Vulkan/D3D12 后端使用 GetNativeHandle 自行创建 Surface/SwapChain）
};

// 窗口配置
struct FWindowConfig {
    std::string title = "ToyEngine";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool resizable = true;
    EWindowGraphicsAPI graphicsAPI = EWindowGraphicsAPI::OpenGL;
};

// 窗口事件回调类型定义
using WindowResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
using CursorPosCallback = std::function<void(double xpos, double ypos)>;
using MouseButtonCallback = std::function<void(int button, int action, int mods)>;
using ScrollCallback = std::function<void(double xoffset, double yoffset)>;

enum class CursorMode
{
    Normal,
    Hidden,
    Disabled
};

// 窗口抽象接口
class IWindow {
public:
    virtual ~IWindow() = default;

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
    virtual void SetCursorPosCallback(CursorPosCallback callback) = 0;
    virtual void SetMouseButtonCallback(MouseButtonCallback callback) = 0;
    virtual void SetScrollCallback(ScrollCallback callback) = 0;
    virtual void SetCursorMode(CursorMode mode) = 0;
    [[nodiscard]] virtual CursorMode GetCursorMode() const = 0;

    /// 交换前后缓冲区。仅 OpenGL 后端有效，Vulkan/D3D12 后端通过各自的 SwapChain 机制提交。
    virtual void SwapBuffers() {}

    /// 设置垂直同步。仅 OpenGL 后端有效。
    virtual void SetVSync(bool enabled) { (void)enabled; }

    /// 查询当前 VSync 状态
    [[nodiscard]] virtual bool IsVSyncEnabled() const { return false; }

    /// 更新窗口标题
    virtual void SetTitle(const std::string& title) { (void)title; }

    /// 查询窗口创建时使用的图形 API 类型
    [[nodiscard]] virtual EWindowGraphicsAPI GetGraphicsAPI() const = 0;

    // 工厂方法 - 根据平台自动创建合适的窗口实现
    [[nodiscard]] static std::unique_ptr<IWindow> Create(const FWindowConfig& config = FWindowConfig());

protected:
    IWindow() = default;
};

} // namespace TE
