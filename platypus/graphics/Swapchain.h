#pragma once

#include "Context.h"
#include "platypus/core/Window.h"
#include "RenderPass.h"
#include <platypus/Common.h>


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
        friend class Context;
        friend class RenderPass;
        SwapchainImpl* _pImpl = nullptr;

        RenderPass _renderPass;

        uint32_t _imageCount = 0;

        // The latest image's index got using acquireImage
        uint32_t _currentImageIndex = 0;

    public:
        Swapchain(Window& window);
        ~Swapchain();

        void create(Window& window);
        void destroy();

        AcquireSwapchainImageResult acquireImage();

        void present(size_t frame);

        size_t getMaxFramesInFlight() const;
        Extent2D getExtent() const;
        inline const RenderPass& getRenderPass() const { return _renderPass; }
        inline uint32_t getImageCount() const { return _imageCount; }
        inline uint32_t getCurrentImageIndex() const { return _currentImageIndex; }
        inline const SwapchainImpl* getPImpl() const { return _pImpl; }
    };
}
