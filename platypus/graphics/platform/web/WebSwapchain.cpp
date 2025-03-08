#include "platypus/graphics/Swapchain.h"


namespace platypus
{
    struct SwapchainImpl
    {
    };


    Swapchain::Swapchain(const Window& window)
    {
    }

    Swapchain::~Swapchain()
    {
        destroy();
    }

    void Swapchain::create(const Window& window)
    {
        _renderPass.create(*this);
    }

    void Swapchain::destroy()
    {
        _renderPass.destroy();
    }

    void Swapchain::recreate(const Window& window)
    {
        destroy();
        create(window);
    }

    SwapchainResult Swapchain::acquireImage()
    {
        return SwapchainResult::SUCCESS;
    }

    SwapchainResult Swapchain::present()
    {
        return SwapchainResult::SUCCESS;
    }

    size_t Swapchain::getMaxFramesInFlight() const
    {
        return 1;
    }

    Extent2D Swapchain::getExtent() const
    {
    }
}
