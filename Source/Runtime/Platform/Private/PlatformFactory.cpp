// ToyEngine Platform Module
// 平台工厂 - 根据平台创建合适的实现

#include "../Public/Window.h"
#include "GLFW/GLFWWindow.h"
#include <memory>

namespace TE {

std::unique_ptr<Window> Window::Create(const WindowConfig& config)
{
    // 目前只有GLFW实现  todo Windows原生窗口、SDL、Qt等
    return std::make_unique<GLFWWindow>(config);
}

} // namespace TE


