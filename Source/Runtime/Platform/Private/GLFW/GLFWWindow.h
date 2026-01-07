// ToyEngine Platform Module
// GLFW窗口实现 - 基于GLFW的跨平台窗口

#pragma once
#include "Window.h"

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

    [[nodiscard]] uint32_t GetWidth() const override { return m_width; }
    [[nodiscard]] uint32_t GetHeight() const override { return m_height; }
    [[nodiscard]] const std::string& GetTitle() const override { return m_title; }

    [[nodiscard]] void* GetNativeHandle() const override;

    // 实现回调设置
    void SetResizeCallback(WindowResizeCallback callback) override;
    void SetCloseCallback(WindowCloseCallback callback) override;
    void SetFocusCallback(WindowFocusCallback callback) override;
    void SetIconifyCallback(WindowIconifyCallback callback) override;
    void SetKeyCallback(KeyCallback callback) override;
    void SetCursorVisible(bool visible) override;

private:
    // GLFW窗口句柄
    GLFWwindow* m_window = nullptr;

    // 窗口属性
    std::string m_title;
    uint32_t m_width;
    uint32_t m_height;

    // 回调函数存储
    WindowResizeCallback m_resizeCallback;
    WindowCloseCallback m_CloseCallback;
    WindowFocusCallback m_FocusCallback;
    WindowIconifyCallback m_iconifyCallback;
    KeyCallback m_keyCallback;


    // 内部使用的静态GLFW回调
    static void GLFWFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void GLFWWindowCloseCallback(GLFWwindow* window);
    static void GLFWWindowFocusCallback(GLFWwindow* window, int focused);
    static void GLFWWindowIconifyCallback(GLFWwindow* window, int iconified);
    static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // 初始化/清理GLFW（静态，全局只初始化一次）
    static void InitializeGLFW();
    static void ShutdownGLFW();
    static int s_glfwWindowCount;
};

} // namespace TE


