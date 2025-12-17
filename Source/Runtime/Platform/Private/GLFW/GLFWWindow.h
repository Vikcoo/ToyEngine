// ToyEngine Platform Module
// GLFW窗口实现 - 基于GLFW的跨平台窗口

#pragma once
#include "../../Public/Window.h"

// 前向声明，避免在头文件中暴露GLFW
struct GLFWwindow;

namespace TE {

// GLFW窗口实现
class GLFWWindow final : public Window {
public:
    explicit GLFWWindow(const WindowConfig& config);
    ~GLFWWindow() override;

    // 实现Window接口
    void Show() override;
    void Hide() override;
    void PollEvents() override;
    [[nodiscard]] bool ShouldClose() const override;

    [[nodiscard]] uint32_t GetWidth() const override { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const override { return m_Height; }
    [[nodiscard]] const std::string& GetTitle() const override { return m_Title; }

    [[nodiscard]] void* GetNativeHandle() const override;

    // 实现回调设置
    void SetResizeCallback(WindowResizeCallback callback) override;
    void SetCloseCallback(WindowCloseCallback callback) override;
    void SetFocusCallback(WindowFocusCallback callback) override;
    void SetIconifyCallback(WindowIconifyCallback callback) override;

private:
    // GLFW窗口句柄
    GLFWwindow* m_Window = nullptr;

    // 窗口属性
    std::string m_Title;
    uint32_t m_Width;
    uint32_t m_Height;

    // 回调函数存储
    WindowResizeCallback m_ResizeCallback;
    WindowCloseCallback m_CloseCallback;
    WindowFocusCallback m_FocusCallback;
    WindowIconifyCallback m_IconifyCallback;

    // 内部使用的静态GLFW回调
    static void GLFWFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void GLFWWindowCloseCallback(GLFWwindow* window);
    static void GLFWWindowFocusCallback(GLFWwindow* window, int focused);
    static void GLFWWindowIconifyCallback(GLFWwindow* window, int iconified);

    // 初始化/清理GLFW（静态，全局只初始化一次）
    static void InitializeGLFW();
    static void ShutdownGLFW();
    static int s_GLFWWindowCount;
};

} // namespace TE


