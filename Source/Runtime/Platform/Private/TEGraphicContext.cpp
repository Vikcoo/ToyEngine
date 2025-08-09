#include "TEGraphicContext.h"

namespace TE {



	std::unique_ptr<TE::TEGraphicContext> TEGraphicContext::Create(TEWindow* window)
	{
#ifdef TE_USE_OPENGL
		return std::unique_ptr<TE::TEGraphicContext>();
#endif

#ifdef TE_USE_VULKAN
		return std::unique_ptr<TE::TEGraphicContext>();
#endif

#ifdef TE_USE_DIRECTX
		return std::unique_ptr<TE::TEGraphicContext>();
#endif

		return nullptr;
	}

}