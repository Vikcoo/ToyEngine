// ToyEngine Platform Module
// GLFW窗口实现 - 基于GLFW的跨平台窗口 + OpenGL Context

#pragma once
#include "Window.h"

struct GLFWwindow;

namespace TE {

class GLFWWindow final : public IWindow {
public:
    explicit GLFWWindow(const FWindowConfig& config);
    ~GLFWWindow() override;

    void PollEvents() override;
    [[nodiscard]] bool ShouldClose() const override;

    [[nodiscard]] uint32_t GetWidth() const override { return m_width; }
    [[nodiscard]] uint32_t GetHeight() const override { return m_height; }
    [[nodiscard]] uint32_t GetFramebufferWidth() const override { return m_fbWidth; }
    [[nodiscard]] uint32_t GetFramebufferHeight() const override { return m_fbHeight; }
    [[nodiscard]] void* GetNativeHandle() const override;

    void SetResizeCallback(WindowResizeCallback callback) override;
    void SetKeyCallback(KeyCallback callback) override;
    void SetCursorPosCallback(CursorPosCallback callback) override;
    void SetMouseButtonCallback(MouseButtonCallback callback) override;
    void SetScrollCallback(ScrollCallback callback) override;
    void SetCursorMode(CursorMode mode) override;
    [[nodiscard]] CursorMode GetCursorMode() const override;

    void SwapBuffers() override;
    void SetVSync(bool enabled) override;
    [[nodiscard]] bool IsVSyncEnabled() const override;
    void SetTitle(const std::string& title) override;
    [[nodiscard]] EWindowGraphicsAPI GetGraphicsAPI() const override { return m_graphicsAPI; }

private:
    GLFWwindow* m_window = nullptr;
    std::string m_title;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_fbWidth = 0;   // 帧缓冲区物理像素宽度
    uint32_t m_fbHeight = 0;  // 帧缓冲区物理像素高度
    bool m_vsyncEnabled = true;
    CursorMode m_cursorMode = CursorMode::Normal;
    EWindowGraphicsAPI m_graphicsAPI = EWindowGraphicsAPI::OpenGL;

    WindowResizeCallback m_resizeCallback;
    KeyCallback m_keyCallback;
    CursorPosCallback m_cursorPosCallback;
    MouseButtonCallback m_mouseButtonCallback;
    ScrollCallback m_scrollCallback;

    // GLFW 静态回调
    static void GLFWFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void GLFWCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void GLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

} // namespace TE
