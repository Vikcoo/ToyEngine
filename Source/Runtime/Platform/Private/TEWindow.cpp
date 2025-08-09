#include "TEWindow.h"
#include "TEGLFWWindow.h"
namespace TE
{
	static std::unique_ptr<TE::TEWindow> Window = nullptr;


	std::unique_ptr<TE::TEWindow> TEWindow::Create(const uint32_t width, const uint32_t height, const char* title)
	{
#ifdef TE_PLATFORM_WIN32
		return std::make_unique<TE::TEGLFWWindow>(width, height, title);
#endif 

#ifdef TE_PLATFORM_LINUX
		return std::make_unique<TE::TEGLFWWindow>(width, height, title);
#endif 

#ifdef TE_PLATFORM_MACOS
		return std::make_unique<TE::TEGLFWWindow>(width, height, title);
#endif 
		return nullptr;
	}
}
