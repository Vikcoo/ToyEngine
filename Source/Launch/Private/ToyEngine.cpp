#include "ToyEngine.h"
#include "TELog.h"
#include "TEWindow.h"
#include "TEGraphicContext.h"
using namespace std;

void ToyEngine::Run()
{
	TE::TELog::Init();
	LOG_TRACE("TEST: {0}, {1}, {2}", __FUNCTION__, 1.11, true);
	LOG_DEBUG("TEST: {0}, {1}, {2}", __FUNCTION__, 1.11, true);
	LOG_INFO("TEST: {0}, {1}, {2}", __FUNCTION__, 1.11, true);
	LOG_WARN("TEST: {0}, {1}, {2}", __FUNCTION__, 1.11, true);
	LOG_ERROR("TEST: {0}, {1}, {2}", __FUNCTION__, 1.11, true);


	unique_ptr<TE::TEWindow> window = TE::TEWindow::Create(1280, 720, "ToyEngine");
	unique_ptr<TE::TEGraphicContext> graphic_context = TE::TEGraphicContext::Create(window.get());


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