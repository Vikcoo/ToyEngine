#include "TEGLFWWindow.h"
#include "TELog.h"
#include "GLFW/glfw3native.h"

namespace TE
{
    TEGLFWWindow::TEGLFWWindow(const uint32_t width, const uint32_t height, const char* title)
    {
        if (!glfwInit())
        {
			LOG_ERROR("Failed to initialize GLFW");
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// 暂时隐藏窗口，待窗口位置设置完成后再显示
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_GLFWWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (m_GLFWWindow == nullptr)
        {
            LOG_ERROR("GLFW Window create failed");
        }

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

		// OPENGL垂直同步默认开启
		// glfwSwapInterval(0); 

		glfwMakeContextCurrent(m_GLFWWindow);

		//初始化结束 显示窗口
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