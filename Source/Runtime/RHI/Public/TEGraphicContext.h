/*
文件用途: 图形上下文抽象层
- 统一抽象不同图形API(Vulkan/GL/DX)的创建与生命周期管理
- Create 工厂接口会根据编译宏(TE_VULKAN/TE_OPENGL/TE_DIRECTX)返回具体实现
- Vulkan 实现在 TEVKGraphicContext，负责 Instance/Surface/PhysicalDevice 等
*/
#pragma once
#include <memory>
namespace TE {
	class TEWindow;
	class TEGraphicContext
	{
	public:
		TEGraphicContext(const TEGraphicContext&) = delete;
		TEGraphicContext& operator=(const TEGraphicContext&) = delete;
		virtual ~TEGraphicContext() = default;

		static std::unique_ptr<TE::TEGraphicContext> Create(TEWindow& window);

	protected:
		TEGraphicContext() = default;
	};
}