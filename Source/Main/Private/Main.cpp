// Created by yu kai on 2025/8/23.
#include <iostream>

#include "TEFileUtil.h"
#include "TEGraphicContext.h"
#include "TELog.h"
#include "TEWindow.h"
#include "Vulkan/TEVKCommandBuffer.h"
#include "Vulkan/TEVKGraphicContext.h"
#include "Vulkan/TEVKLogicDevice.h"
#include "Vulkan/TEVKRenderPass.h"
#include "Vulkan/TEVKSwapChain.h"
#include "Vulkan/TEVKFrameBuffer.h"
#include "Vulkan/TEVKPipeline.h"
#include "Vulkan/TEVKQueue.h"

int main(){
    TE::TELog::Init();
    std::unique_ptr<TE::TEWindow> window = TE::TEWindow::Create(1280, 720, "ToyEngine");
    std::unique_ptr<TE::TEGraphicContext> graphicContext = TE::TEGraphicContext::Create(*window);
    auto& vkContext = dynamic_cast<TE::TEVKGraphicContext&>(*graphicContext);
    std::shared_ptr<TE::TEVKLogicDevice> device = std::make_shared<TE::TEVKLogicDevice>(vkContext,1,1);
    std::shared_ptr<TE::TEVKSwapChain> swapChain = std::make_shared<TE::TEVKSwapChain>(vkContext,*device);
    std::shared_ptr<TE::TEVKRenderPass> renderPass = std::make_shared<TE::TEVKRenderPass>(*device);
    auto images = swapChain->GetImages();
    std::vector<std::shared_ptr<TE::TEVKFrameBuffer>> frameBuffers;
    for (const auto& image: images) {
        std::vector<vk::Image> tempImages{image};
        frameBuffers.push_back(std::make_shared<TE::TEVKFrameBuffer>(*device, *renderPass, tempImages,swapChain->GetWidth(),swapChain->GetHeight()));
    }

    std::shared_ptr<TE::TEVKPipelineLayout> layout = std::make_shared<TE::TEVKPipelineLayout>(*device, TE_RES_SHADER_DIR"graphics.vert", TE_RES_SHADER_DIR"graphics.frag");
    std::shared_ptr<TE::TEVKPipeline> pipeline = std::make_shared<TE::TEVKPipeline>(*device, *renderPass, *layout);
    pipeline->SetInputAssemblyState(vk::PrimitiveTopology::eTriangleList)->EnableDepthTest();
    pipeline->SetDynamicState({vk::DynamicState::eViewport, vk::DynamicState::eScissor});
    pipeline->Create();

    std::shared_ptr<TE::TEVKCommandPool> commandPool = std::make_shared<TE::TEVKCommandPool>(*device,vkContext.GetGraphicQueueFamilyInfo().queueFamilyIndex);
    std::vector<vk::raii::CommandBuffer> commandBuffers = commandPool->AllocateCommandBuffer(images.size());

    constexpr vk::SemaphoreCreateInfo semaphoreInfo;
    constexpr vk::FenceCreateInfo fenceInfo(
        vk::FenceCreateFlagBits::eSignaled  // flags
    );

    const uint32_t numBuffer = 2;
    std::vector<vk::raii::Fence> fences;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> submitSemaphores;

    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < numBuffer; ++i) {
        imageAvailableSemaphores.emplace_back(device->GetHandle()->createSemaphore(semaphoreCreateInfo));
        submitSemaphores.emplace_back(device->GetHandle()->createSemaphore(semaphoreCreateInfo));
        fences.emplace_back(device->GetHandle()->createFence(fenceCreateInfo));
    }


    uint32_t currentBuffer = 0;
     std::vector<vk::ClearValue> clearValues = {vk::ClearValue({0.2f, 0.2f, 0.2f, 1.0f})};
    while (!window->IsShouldClose())
    {
        window->PollEvents();
        CALL_VK_CHECK(device->GetHandle()->waitForFences(*fences[currentBuffer], 1, UINT64_MAX));
        device->GetHandle()->resetFences(*fences[currentBuffer]);


        // 1.获取交换链图片
        uint32_t imageIndex = swapChain->AcquireImage(imageAvailableSemaphores[currentBuffer]);
        // 2.开启命令缓冲
        TE::TEVKCommandPool::BeginCommandBuffer(commandBuffers[imageIndex]);
        // 3.开启 render pass，绑定frame buffer
        renderPass->Begin(commandBuffers[imageIndex],*frameBuffers[imageIndex],clearValues);
        // 4.绑定资源
        pipeline->Bind(commandBuffers[imageIndex]);

        vk::Viewport viewport;
        viewport.setX(0).setY(0).setWidth( static_cast<float>(frameBuffers[imageIndex]->GetWidth())).setHeight( static_cast<float>(frameBuffers[imageIndex]->GetHeight()));
        commandBuffers[imageIndex].setViewport(0,viewport);
        vk::Rect2D scissor;
        scissor.setOffset({ 0, 0 }).setExtent({ frameBuffers[imageIndex]->GetWidth(), frameBuffers[imageIndex]->GetHeight() });
        commandBuffers[imageIndex].setScissor(0,scissor);
        // 5.draw
        commandBuffers[imageIndex].draw(3, 1, 0, 0);
        // 6.end render pass
        renderPass->End(commandBuffers[imageIndex]);
        // 7.结束command buffer
        TE::TEVKCommandPool::EndCommandBuffer(commandBuffers[imageIndex]);
        // 8.提交command buffer到queue
        std::vector<vk::CommandBuffer> rawCmdBuffers;
        rawCmdBuffers.push_back(*commandBuffers[imageIndex]);  // 解引用RAII对象获取原始句柄
        device->GetFirstGraphicQueue()->Submit({rawCmdBuffers},{imageAvailableSemaphores[currentBuffer]},{submitSemaphores[currentBuffer]},fences[currentBuffer]);

        // 9.present
        swapChain->Present(imageIndex,{submitSemaphores[currentBuffer]});

        window->SwapBuffers();
        currentBuffer = (currentBuffer + 1) % numBuffer;
    }
    return 0;
}
