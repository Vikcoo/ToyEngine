#include "ToyEngine.h"
#include "TELog.h"
#include "TEWindow.h"
#include "TEGraphicContext.h"
#include "TEVKGraphicContext.h"
#include "TEVKLogicDevice.h"
using namespace std;

void ToyEngine::Run()
{
	TE::TELog::Init();

	unique_ptr<TE::TEWindow> window = TE::TEWindow::Create(1280, 720, "ToyEngine");
	unique_ptr<TE::TEGraphicContext> graphic_context = TE::TEGraphicContext::Create(window.get());
	std::shared_ptr<TE::TEVKLogicDevice> logic_device = std::make_shared<TE::TEVKLogicDevice>(dynamic_cast<TE::TEVKGraphicContext*>(graphic_context.get()),1,1);

	while (!window->IsShouldClose())
	{
		window->PollEvents();
		window->SwapBuffers();
	}
}

int main()
{
	ToyEngine::Run();
    return 0;
}