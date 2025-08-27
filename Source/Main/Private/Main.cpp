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
    auto m_imageAvailableSemaphore = device->GetHandle()->createSemaphore( semaphoreInfo );
    auto m_renderFinishedSemaphore = device->GetHandle()->createSemaphore( semaphoreInfo );
    auto m_inFlightFence = device->GetHandle()->createFence( fenceInfo );

    while (!window->IsShouldClose())
    {
        window->PollEvents();

        if(const auto res = device->GetHandle()->waitForFences( *m_inFlightFence, true, std::numeric_limits<uint64_t>::max() );
           res != vk::Result::eSuccess
       ) throw std::runtime_error{ "waitForFences in drawFrame was failed" };

        device->GetHandle()->resetFences( *m_inFlightFence );

        auto [nxtRes, imageIndex] = swapChain->GetHandle().acquireNextImage(std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphore);

        commandBuffers[0].reset();



        constexpr vk::CommandBufferBeginInfo beginInfo;
        commandBuffers[0].begin( beginInfo );

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass->GetHandle();
        renderPassInfo.framebuffer = frameBuffers[imageIndex]->GetHandle();
        renderPassInfo.renderArea.setOffset(vk::Offset2D{0, 0});
        renderPassInfo.renderArea.setExtent({frameBuffers[imageIndex]->GetWidth(),frameBuffers[imageIndex]->GetHeight()});
        constexpr vk::ClearValue clearColor(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
        renderPassInfo.setClearValues( clearColor );

        commandBuffers[0].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffers[0].bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline->GetHandle() );

        const vk::Viewport viewport(
            0.0f, 0.0f, // x, y
            static_cast<float>(frameBuffers[imageIndex]->GetWidth()),    // width
            static_cast<float>(frameBuffers[imageIndex]->GetHeight()),   // height
            0.0f, 1.0f  // minDepth maxDepth
        );
        commandBuffers[0].setViewport(0, viewport);

        const vk::Rect2D scissor(
            vk::Offset2D{0, 0}, // offset
            vk::Extent2D{frameBuffers[imageIndex]->GetWidth(), frameBuffers[imageIndex]->GetHeight()}   // extent
        );
        commandBuffers[0].setScissor(0, scissor);

        commandBuffers[0].draw(3, 1, 0, 0);

        commandBuffers[0].endRenderPass();
        commandBuffers[0].end();








        vk::SubmitInfo submitInfo;
        submitInfo.setWaitSemaphores( *m_imageAvailableSemaphore );
        std::array<vk::PipelineStageFlags,1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.setWaitDstStageMask( waitStages );
        submitInfo.setCommandBuffers( *commandBuffers[0] );

        submitInfo.setSignalSemaphores( *m_renderFinishedSemaphore );
        device->GetFirstGraphicQueue()->GetHandle().submit(submitInfo, m_inFlightFence);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores( *m_renderFinishedSemaphore );
        presentInfo.setSwapchains( *swapChain->GetHandle() );
        presentInfo.pImageIndices = &imageIndex;
        if (const auto res =  device->GetFirstPresentQueue()->GetHandle().presentKHR( presentInfo );
            res != vk::Result::eSuccess
        ) throw std::runtime_error{ "presentKHR in drawFrame was failed" };

        // // 1.获取交换链图片
        // uint32_t imageIndex = swapChain->AcquireImage();
        // // 2.开启命令缓冲
        // TE::TEVKCommandPool::BeginCommandBuffer(commandBuffers[imageIndex]);
        // // 3.开启 render pass，绑定frame buffer
        // renderPass->Begin(commandBuffers[imageIndex],*frameBuffers[imageIndex],clearValues);
        // // 4.绑定资源
        // pipeline->Bind(commandBuffers[imageIndex]);
        //
        // vk::Viewport viewport;
        // viewport.setX(0).setY(0).setWidth( static_cast<float>(frameBuffers[imageIndex]->GetWidth())).setHeight( static_cast<float>(frameBuffers[imageIndex]->GetHeight()));
        // commandBuffers[imageIndex].setViewport(0,viewport);
        // vk::Rect2D scissor;
        // scissor.setOffset({ 0, 0 }).setExtent({ frameBuffers[imageIndex]->GetWidth(), frameBuffers[imageIndex]->GetHeight() });
        // commandBuffers[imageIndex].setScissor(0,scissor);
        // // 5.draw
        // commandBuffers[imageIndex].draw(3, 1, 0, 0);
        // // 6.end render pass
        // renderPass->End(commandBuffers[imageIndex]);
        // // 7.结束command buffer
        // TE::TEVKCommandPool::EndCommandBuffer(commandBuffers[imageIndex]);
        // // 8.提交command buffer到queue
        // std::vector<vk::CommandBuffer> rawCmdBuffers;
        // rawCmdBuffers.push_back(*commandBuffers[imageIndex]);  // 解引用RAII对象获取原始句柄
        // device->GetFirstGraphicQueue()->Submit({rawCmdBuffers});
        // device->GetFirstGraphicQueue()->WaitIdle();
        // // 9.present
        // swapChain->Present(imageIndex);

        window->SwapBuffers();
    }
    return 0;
}
