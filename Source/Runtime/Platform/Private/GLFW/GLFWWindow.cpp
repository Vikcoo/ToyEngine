// ToyEngine Platform Module
// GLFW窗口实现

#include "GLFWWindow.h"
#include "Log/Log.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace TE {

// 静态成员初始化
int GLFWWindow::s_glfwWindowCount = 0;

// GLFW错误回调
static void GLFWErrorCallback(int error, const char* description)
{
    TE_LOG_ERROR("[GLFW Error {}]: {}", error, description);
}

void GLFWWindow::InitializeGLFW()
{
    if (s_glfwWindowCount == 0)
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
    s_glfwWindowCount++;
}

void GLFWWindow::ShutdownGLFW()
{
    s_glfwWindowCount--;
    if (s_glfwWindowCount == 0)
    {
        TE_LOG_INFO("[Platform] Shutting down GLFW...");
        glfwTerminate();
    }
}

GLFWWindow::GLFWWindow(const WindowConfig& config)
    : m_title(config.title)
    , m_width(config.width)
    , m_height(config.height)
{
    InitializeGLFW();

    // 配置GLFW窗口提示
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // 不创建OpenGL上下文（为Vulkan准备）
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // 创建窗口
    m_window = glfwCreateWindow(
        static_cast<int>(m_width),
        static_cast<int>(m_height),
        m_title.c_str(),
        nullptr,
        nullptr
    );

    if (!m_window)
    {
        ShutdownGLFW();
        TE_LOG_CRITICAL("[Platform] Failed to create GLFW window!");
        throw std::runtime_error("Failed to create GLFW window!");
    }

    TE_LOG_INFO("[Platform] Window created: \"{}\" ({}x{})", m_title, m_width, m_height);

    // 将this指针存储到GLFW窗口，以便在静态回调中访问
    glfwSetWindowUserPointer(m_window, this);

    // 设置GLFW事件回调
    glfwSetFramebufferSizeCallback(m_window, GLFWFramebufferSizeCallback);
    glfwSetWindowCloseCallback(m_window, GLFWWindowCloseCallback);
    glfwSetWindowFocusCallback(m_window, GLFWWindowFocusCallback);
    glfwSetWindowIconifyCallback(m_window, GLFWWindowIconifyCallback);
    glfwSetKeyCallback(m_window, GLFWKeyCallback);
}

GLFWWindow::~GLFWWindow()
{
    if (m_window)
    {
        TE_LOG_INFO("[Platform] Destroying window...");
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    ShutdownGLFW();
}

void GLFWWindow::Show()
{
    if (m_window)
    {
        glfwShowWindow(m_window);
    }
}

void GLFWWindow::Hide()
{
    if (m_window)
    {
        glfwHideWindow(m_window);
    }
}

bool GLFWWindow::ShouldClose() const
{
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void GLFWWindow::PollEvents()
{
    glfwPollEvents();
}

void* GLFWWindow::GetNativeHandle() const
{
    return m_window;
}

// 实现用户回调设置
void GLFWWindow::SetResizeCallback(WindowResizeCallback callback)
{
    m_resizeCallback = std::move(callback);
}

void GLFWWindow::SetCloseCallback(WindowCloseCallback callback)
{
    m_CloseCallback = std::move(callback);
}

void GLFWWindow::SetFocusCallback(WindowFocusCallback callback)
{
    m_FocusCallback = std::move(callback);
}

void GLFWWindow::SetIconifyCallback(WindowIconifyCallback callback)
{
    m_iconifyCallback = std::move(callback);
}

void GLFWWindow::SetKeyCallback(KeyCallback callback)
{
    m_keyCallback = std::move(callback);
}

void GLFWWindow::SetCursorVisible(bool visible)
{
    if (visible){
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }else{
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

}

// GLFW静态回调 -> 调用用户回调
void GLFWWindow::GLFWFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window)))
    {
        // 更新内部尺寸
        self->m_width = static_cast<uint32_t>(width);
        self->m_height = static_cast<uint32_t>(height);
        
        TE_LOG_DEBUG("[Platform] Window resized: {}x{}", width, height);
        
        // 触发用户回调
        if (self->m_resizeCallback)
        {
            self->m_resizeCallback(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
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

void GLFWWindow::GLFWWindowIconifyCallback(GLFWwindow* window, int iconified)
{
    TE_LOG_DEBUG("[Platform] Window IconifyCallback");
    const auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_iconifyCallback)
    {
        self->m_iconifyCallback(iconified == GLFW_TRUE);
    }
}

void GLFWWindow::GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    TE_LOG_DEBUG("[Platform] Window KeyCallback");
    const auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_keyCallback)
    {
        self->m_keyCallback(key, scancode, action, mods);
    }
}
} // namespace TE

