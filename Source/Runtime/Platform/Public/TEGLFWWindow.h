#pragma once
#include "TEWindow.h"
#include "GLFW/glfw3.h"

namespace TE
{
	class TEGLFWWindow : public TEWindow
	{
	public:
		TEGLFWWindow() = delete;
        // 修复构造函数参数中的中文逗号为英文逗号
        TEGLFWWindow(const uint32_t width, const uint32_t height, const char *title);
		bool IsShouldClose() override;
		void PollEvents() override;
		void SwapBuffers() override;
		virtual ~TEGLFWWindow() override;
		GLFWwindow* GetGLFWWindowHandle() const {
			return m_GLFWWindow;
		};
	private:
		GLFWwindow* m_GLFWWindow = nullptr;
	};
}