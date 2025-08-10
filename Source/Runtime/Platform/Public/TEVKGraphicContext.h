#pragma once
#include <memory>
#include "TEGraphicContext.h"
#include "TEVKCommon.h"


namespace TE {
	class TEVKGraphicContext : public TEGraphicContext
	{
	public:
		TEVKGraphicContext(TEWindow* window);
		~TEVKGraphicContext() override;
	private:
		void CreateInstance();
		vk::Instance m_Instance;
	};
}