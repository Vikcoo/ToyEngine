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