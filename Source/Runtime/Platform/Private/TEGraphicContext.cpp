#include "TEGraphicContext.h"

#include "TEVKGraphicContext.h"

namespace TE {



	std::unique_ptr<TE::TEGraphicContext> TEGraphicContext::Create(TEWindow* window)
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
