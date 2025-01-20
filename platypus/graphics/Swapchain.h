#pragma once

#include "Context.h"
#include "platypus/core/Window.h"


namespace platypus
{
    struct SwapchainImpl;

    class Swapchain
    {
    private:
        SwapchainImpl* _pImpl = nullptr;

    public:
        Swapchain(Window& window);
        ~Swapchain();

        void create(Window& window);
        void destroy();
    };
}
