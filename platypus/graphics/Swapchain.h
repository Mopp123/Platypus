#pragma once

#include "Context.h"
#include "platypus/core/Window.h"
#include "RenderPass.h"


namespace platypus
{
    struct SwapchainImpl;

    enum class AcquireSwapchainImageResult
    {
        ERROR,
        RESIZE_REQUIRED,
        SUCCESS
    };

    class Swapchain
    {
    private:
        friend class RenderPass;
        SwapchainImpl* _pImpl = nullptr;

        RenderPass _renderPass;

    public:
        Swapchain(Window& window);
        ~Swapchain();

        void create(Window& window);
        void destroy();

        AcquireSwapchainImageResult acquireImage(uint32_t* pOutImageIndex);
    };
}
