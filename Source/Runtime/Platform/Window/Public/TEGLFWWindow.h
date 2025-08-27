#pragma once
#include "TEWindow.h"

class GLFWwindow;
namespace TE
{

	class TEGLFWWindow : public TEWindow
	{
	public:
		TEGLFWWindow() = delete;
        // 修复构造函数参数中的中文逗号为英文逗号
        TEGLFWWindow(int width, int height, const char *title);
		bool IsShouldClose() override;
		void PollEvents() override;
		void SwapBuffers() override;
		virtual ~TEGLFWWindow() override;
		[[nodiscard]] GLFWwindow* GetGLFWWindowHandle() const {
			return m_GLFWWindow;
		};
	private:
		GLFWwindow* m_GLFWWindow = nullptr;
	};
}