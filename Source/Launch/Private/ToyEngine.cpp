#include "ToyEngine.h"
#include "TELog.h"
#include "TEWindow.h"
#include "TEGraphicContext.h"
#include "TEVKFrameBuffer.h"
#include "TEVKGraphicContext.h"
#include "TEVKLogicDevice.h"
#include "TEVKSwapChain.h"
#include "TEVKRenderPass.h"
using namespace std;

void ToyEngine::Run()
{
	TE::TELog::Init();

	unique_ptr<TE::TEWindow> window = TE::TEWindow::Create(1280, 720, "ToyEngine");
	unique_ptr<TE::TEGraphicContext> vnContext = TE::TEGraphicContext::Create(window.get());
	std::shared_ptr<TE::TEVKLogicDevice> logicDevice = std::make_shared<TE::TEVKLogicDevice>(dynamic_cast<TE::TEVKGraphicContext*>(vnContext.get()),1,1);
	std::shared_ptr<TE::TEVKSwapChain> swapChain = std::make_shared<TE::TEVKSwapChain>(dynamic_cast<TE::TEVKGraphicContext*>(vnContext.get()),logicDevice.get());
	//旧的chain没销毁？
	swapChain->ReCreate();
	std::shared_ptr<TE::TEVKRenderPass> renderPass = std::make_shared<TE::TEVKRenderPass>(logicDevice.get());

	std::vector<vk::Image> swapChainImages = swapChain->GetImages();
	std::vector<std::shared_ptr<TE::TEVKFrameBuffer>> frameBuffers;
	for (const auto& image: swapChainImages) {
		std::vector<vk::Image> images = { image };
		frameBuffers.push_back(std::make_shared<TE::TEVKFrameBuffer>(logicDevice.get(), renderPass.get(), images,swapChain->GetWidth(), swapChain->GetHeight()));
	}


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