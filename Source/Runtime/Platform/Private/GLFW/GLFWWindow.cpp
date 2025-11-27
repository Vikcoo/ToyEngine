// ToyEngine Platform Module
// GLFW窗口实现

#include "GLFWWindow.h"
#include "Log/Log.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace TE {

// 静态成员初始化
int GLFWWindow::s_GLFWWindowCount = 0;

// GLFW错误回调
static void GLFWErrorCallback(int error, const char* description)
{
    TE_LOG_ERROR("[GLFW Error {}]: {}", error, description);
}

void GLFWWindow::InitializeGLFW()
{
    if (s_GLFWWindowCount == 0)
    {
        TE_LOG_INFO("[Platform] Initializing GLFW...");
        
        // 设置错误回调
        glfwSetErrorCallback(GLFWErrorCallback);
        
        if (!glfwInit())
        {
            TE_LOG_CRITICAL("[Platform] Failed to initialize GLFW!");
            throw std::runtime_error("Failed to initialize GLFW!");
        }
        
        TE_LOG_INFO("[Platform] GLFW initialized successfully");
    }
    s_GLFWWindowCount++;
}

void GLFWWindow::ShutdownGLFW()
{
    s_GLFWWindowCount--;
    if (s_GLFWWindowCount == 0)
    {
        TE_LOG_INFO("[Platform] Shutting down GLFW...");
        glfwTerminate();
    }
}

GLFWWindow::GLFWWindow(const WindowConfig& config)
    : m_Title(config.title)
    , m_Width(config.width)
    , m_Height(config.height)
{
    InitializeGLFW();

    // 配置GLFW窗口提示
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // 不创建OpenGL上下文（为Vulkan准备）
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // 创建窗口
    m_Window = glfwCreateWindow(
        static_cast<int>(m_Width),
        static_cast<int>(m_Height),
        m_Title.c_str(),
        nullptr,
        nullptr
    );

    if (!m_Window)
    {
        ShutdownGLFW();
        TE_LOG_CRITICAL("[Platform] Failed to create GLFW window!");
        throw std::runtime_error("Failed to create GLFW window!");
    }

    TE_LOG_INFO("[Platform] Window created: \"{}\" ({}x{})", m_Title, m_Width, m_Height);

    // 将this指针存储到GLFW窗口，以便在静态回调中访问
    glfwSetWindowUserPointer(m_Window, this);

    // 设置GLFW事件回调
    glfwSetFramebufferSizeCallback(m_Window, GLFWFramebufferSizeCallback);
    glfwSetWindowCloseCallback(m_Window, GLFWWindowCloseCallback);
    glfwSetWindowFocusCallback(m_Window, GLFWWindowFocusCallback);
}

GLFWWindow::~GLFWWindow()
{
    if (m_Window)
    {
        TE_LOG_INFO("[Platform] Destroying window...");
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    ShutdownGLFW();
}

void GLFWWindow::Show()
{
    if (m_Window)
    {
        glfwShowWindow(m_Window);
    }
}

void GLFWWindow::Hide()
{
    if (m_Window)
    {
        glfwHideWindow(m_Window);
    }
}

bool GLFWWindow::ShouldClose() const
{
    return m_Window ? glfwWindowShouldClose(m_Window) : true;
}

void GLFWWindow::PollEvents()
{
    glfwPollEvents();
}

void* GLFWWindow::GetNativeHandle() const
{
    return m_Window;
}

// 实现用户回调设置
void GLFWWindow::SetResizeCallback(WindowResizeCallback callback)
{
    m_ResizeCallback = std::move(callback);
}

void GLFWWindow::SetCloseCallback(WindowCloseCallback callback)
{
    m_CloseCallback = std::move(callback);
}

void GLFWWindow::SetFocusCallback(WindowFocusCallback callback)
{
    m_FocusCallback = std::move(callback);
}

// GLFW静态回调 -> 调用用户回调
void GLFWWindow::GLFWFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window)))
    {
        // 更新内部尺寸
        self->m_Width = static_cast<uint32_t>(width);
        self->m_Height = static_cast<uint32_t>(height);
        
        TE_LOG_DEBUG("[Platform] Window resized: {}x{}", width, height);
        
        // 触发用户回调
        if (self->m_ResizeCallback)
        {
            self->m_ResizeCallback(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }
    }
}

void GLFWWindow::GLFWWindowCloseCallback(GLFWwindow* window)
{
    if (const auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window)); self && self->m_CloseCallback)
    {
        self->m_CloseCallback();
    }
}

void GLFWWindow::GLFWWindowFocusCallback(GLFWwindow* window, int focused)
{
    const auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_FocusCallback)
    {
        self->m_FocusCallback(focused == GLFW_TRUE);
    }
}

} // namespace TE

