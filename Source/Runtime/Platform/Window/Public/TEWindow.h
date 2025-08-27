#pragma once
#include <cassert>
#include <memory>

namespace TE {
	class TEWindow
	{
	public:
		TEWindow(const TEWindow&) = delete;
		TEWindow& operator=(const TEWindow&) = delete;
		virtual ~TEWindow() = default;
		static std::unique_ptr<TE::TEWindow> Create(const uint32_t width, const uint32_t height, const char* title);
		virtual bool IsShouldClose() = 0;
		virtual void PollEvents() = 0;
		virtual void SwapBuffers() = 0;
	protected:
		TEWindow() = default;
	};
}