// ToyEngine Platform Module
// GLFW 窗口实现 - 支持 OpenGL Context 和 No-API 模式（供 Vulkan/D3D12 使用）

#include "GLFWWindow.h"
#include "Log/Log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace TE {

static bool s_glfwInitialized = false;

GLFWWindow::GLFWWindow(const FWindowConfig& config)
    : m_title(config.title), m_width(config.width), m_height(config.height)
    , m_graphicsAPI(config.graphicsAPI)
{
    if (!s_glfwInitialized)
    {
        glfwSetErrorCallback([](int error, const char* desc) {
            TE_LOG_ERROR("[GLFW Error {}]: {}", error, desc);
        });

        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW!");
        }

        s_glfwInitialized = true;
        TE_LOG_INFO("[Platform] GLFW initialized");
    }

    // 根据图形 API 类型设置不同的窗口 hint
    if (config.graphicsAPI == EWindowGraphicsAPI::OpenGL)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef TE_PLATFORM_MACOS
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    }
    else
    {
        // Vulkan/D3D12：不创建图形上下文，后端通过 GetNativeHandle() 自行初始化
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(
        static_cast<int>(m_width),
        static_cast<int>(m_height),
        m_title.c_str(),
        nullptr,
        nullptr
    );

    if (!m_window)
    {
        throw std::runtime_error("Failed to create GLFW window!");
    }

    // OpenGL 模式下初始化上下文和 glad
    if (config.graphicsAPI == EWindowGraphicsAPI::OpenGL)
    {
        glfwMakeContextCurrent(m_window);

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            glfwDestroyWindow(m_window);
            throw std::runtime_error("Failed to initialize glad!");
        }

        TE_LOG_INFO("[Platform] OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        TE_LOG_INFO("[Platform] Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        glfwSwapInterval(1);
        m_vsyncEnabled = true;
    }
    else
    {
        m_vsyncEnabled = false;
        TE_LOG_INFO("[Platform] Window created in No-API mode (Vulkan/D3D12)");
    }

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(m_window, &fbW, &fbH);
    m_fbWidth = static_cast<uint32_t>(fbW);
    m_fbHeight = static_cast<uint32_t>(fbH);

    glfwShowWindow(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, GLFWFramebufferSizeCallback);
    glfwSetKeyCallback(m_window, GLFWKeyCallback);
    glfwSetCursorPosCallback(m_window, GLFWCursorPosCallback);
    glfwSetMouseButtonCallback(m_window, GLFWMouseButtonCallback);
    glfwSetScrollCallback(m_window, GLFWScrollCallback);

    TE_LOG_INFO("[Platform] Window created: \"{}\" ({}x{}, framebuffer: {}x{})",
                m_title, m_width, m_height, m_fbWidth, m_fbHeight);
}

GLFWWindow::~GLFWWindow()
{
    if (m_window)
    {
        TE_LOG_DEBUG("[Platform] Destroying window...");
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
    s_glfwInitialized = false;
}

void GLFWWindow::PollEvents()
{
    glfwPollEvents();
}

bool GLFWWindow::ShouldClose() const
{
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void* GLFWWindow::GetNativeHandle() const
{
    return m_window;
}

void GLFWWindow::SwapBuffers()
{
    glfwSwapBuffers(m_window);
}

void GLFWWindow::SetVSync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
    m_vsyncEnabled = enabled;
    TE_LOG_INFO("[Platform] VSync {}", enabled ? "ON (locked to display refresh rate)" : "OFF (unlocked framerate)");
}

bool GLFWWindow::IsVSyncEnabled() const
{
    return m_vsyncEnabled;
}

void GLFWWindow::SetTitle(const std::string& title)
{
    m_title = title;
    if (m_window)
    {
        glfwSetWindowTitle(m_window, m_title.c_str());
    }
}

void GLFWWindow::SetResizeCallback(WindowResizeCallback callback)
{
    m_resizeCallback = std::move(callback);
}

void GLFWWindow::SetKeyCallback(KeyCallback callback)
{
    m_keyCallback = std::move(callback);
}

void GLFWWindow::SetCursorPosCallback(CursorPosCallback callback)
{
    m_cursorPosCallback = std::move(callback);
}

void GLFWWindow::SetMouseButtonCallback(MouseButtonCallback callback)
{
    m_mouseButtonCallback = std::move(callback);
}

void GLFWWindow::SetScrollCallback(ScrollCallback callback)
{
    m_scrollCallback = std::move(callback);
}

void GLFWWindow::SetCursorMode(CursorMode mode)
{
    if (!m_window)
    {
        return;
    }

    int glfwMode = GLFW_CURSOR_NORMAL;
    switch (mode)
    {
    case CursorMode::Normal:
        glfwMode = GLFW_CURSOR_NORMAL;
        break;
    case CursorMode::Hidden:
        glfwMode = GLFW_CURSOR_HIDDEN;
        break;
    case CursorMode::Disabled:
        glfwMode = GLFW_CURSOR_DISABLED;
        break;
    default:
        glfwMode = GLFW_CURSOR_NORMAL;
        break;
    }

    glfwSetInputMode(m_window, GLFW_CURSOR, glfwMode);
    m_cursorMode = mode;
}

CursorMode GLFWWindow::GetCursorMode() const
{
    return m_cursorMode;
}

// GLFW 静态回调（注意：FramebufferSizeCallback 的参数是帧缓冲区物理像素尺寸）
void GLFWWindow::GLFWFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    // 更新帧缓冲区尺寸
    self->m_fbWidth = static_cast<uint32_t>(width);
    self->m_fbHeight = static_cast<uint32_t>(height);

    // 同时获取逻辑窗口尺寸
    int winW = 0, winH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    self->m_width = static_cast<uint32_t>(winW);
    self->m_height = static_cast<uint32_t>(winH);

    // 仅 OpenGL 模式下同步 Viewport
    if (self->m_graphicsAPI == EWindowGraphicsAPI::OpenGL)
    {
        glViewport(0, 0, width, height);
    }

    if (self->m_resizeCallback)
    {
        self->m_resizeCallback(self->m_fbWidth, self->m_fbHeight);
    }
}

void GLFWWindow::GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_keyCallback)
    {
        self->m_keyCallback(key, scancode, action, mods);
    }
}

void GLFWWindow::GLFWCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_cursorPosCallback)
    {
        self->m_cursorPosCallback(xpos, ypos);
    }
}

void GLFWWindow::GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_mouseButtonCallback)
    {
        self->m_mouseButtonCallback(button, action, mods);
    }
}

void GLFWWindow::GLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_scrollCallback)
    {
        self->m_scrollCallback(xoffset, yoffset);
    }
}

// 工厂方法（内联，不需要单独的 PlatformFactory.cpp）
std::unique_ptr<IWindow> IWindow::Create(const FWindowConfig& config)
{
    return std::make_unique<GLFWWindow>(config);
}

} // namespace TE
