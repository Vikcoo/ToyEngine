#include "TEGLFWWindow.h"
#include "TELog.h"
#include <GLFW/glfw3.h>

namespace TE
{
    TEGLFWWindow::TEGLFWWindow(const int width, const int height, const char* title)
    {
        if (!glfwInit())
        {
			LOG_ERROR("Failed to initialize GLFW");
            return;
        }

    	// 表示这个窗口不使用任何图形 API，如果用 Vulkan，GLFW_NO_API是必须的！
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    	//
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_GLFWWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (m_GLFWWindow == nullptr)
        {
            LOG_ERROR("GLFW Window create failed");
        }

    	//获取主显示器信息并居中窗口
        auto PrimaryMonitor = glfwGetPrimaryMonitor();
        if (PrimaryMonitor)
        {
            int PrimaryMonitorXpos, PrimaryMonitorYpos, PrimaryMonitorWidth, PrimaryMonitorHeight;
            glfwGetMonitorWorkarea(PrimaryMonitor, 
                                   &PrimaryMonitorXpos, 
                                   &PrimaryMonitorYpos, 
                                   &PrimaryMonitorWidth, 
                                   &PrimaryMonitorHeight);
			glfwSetWindowPos(m_GLFWWindow, 
                           PrimaryMonitorXpos + (PrimaryMonitorWidth - width) / 2, 
                           PrimaryMonitorYpos + (PrimaryMonitorHeight - height) / 2);
            LOG_INFO("GLFW Window created at primary monitor position: ({}, {}) with size: {}x{}", 
				PrimaryMonitorXpos, PrimaryMonitorYpos, width, height);
        }
        else
        {
            LOG_ERROR("GLFW Get Primary Monitor failed");
        }

		glfwShowWindow(m_GLFWWindow);
    }

    bool TE::TEGLFWWindow::IsShouldClose() {
        return glfwWindowShouldClose(m_GLFWWindow);
    }

    void TE::TEGLFWWindow::PollEvents() {
		glfwPollEvents();
    }

    void TE::TEGLFWWindow::SwapBuffers() {
		glfwSwapBuffers(m_GLFWWindow);
    }

    TEGLFWWindow::~TEGLFWWindow()
    {
		glfwDestroyWindow(m_GLFWWindow);
		glfwTerminate();
		LOG_INFO("GLFW Window Destroyed");
    }
}
