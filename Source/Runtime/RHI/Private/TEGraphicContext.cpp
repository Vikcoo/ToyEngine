/*
文件用途: 图形上下文工厂实现
- 根据编译宏选择具体的图形后端实现（Vulkan/OpenGL/DirectX）
- Vulkan: 返回 TEVKGraphicContext，内部负责 Instance/Surface/PhysicalDevice 等初始化
- 其他宏分支当前返回占位空指针
*/
#include "TEGraphicContext.h"
#include "../Public/Vulkan/TEVKGraphicContext.h"

namespace TE {
	std::unique_ptr<TE::TEGraphicContext> TEGraphicContext::Create(TEWindow& window)
	{
#ifdef TE_OPENGL
		return std::unique_ptr<TEGraphicContext>();
#endif

#ifdef TE_VULKAN
		return std::make_unique<TEVKGraphicContext>(window);
#endif

#ifdef TE_DIRECTX
		return std::unique_ptr<TEGraphicContext>();
#endif

		return nullptr;
	}

}
