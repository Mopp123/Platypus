#pragma once

#include "platypus/core/Window.hpp"
#include "RenderPass.h"
#include <platypus/Common.h>


namespace platypus
{
    struct SwapchainImpl;

    enum class SwapchainResult
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

        // NOTE: Currently these aren't touched by the web implementation at all.
        // TODO: Maybe put these in the _pImpl?
        uint32_t _imageCount = 0;
        uint32_t _previousImageCount = 0;

        // The latest image's index got using acquireImage
        uint32_t _currentImageIndex = 0;
        size_t _currentFrame = 0;

    public:
        Swapchain(const Window& window);
        ~Swapchain();

        void create(const Window& window);
        void destroy();
        void recreate(const Window& window);

        SwapchainResult acquireImage();
        SwapchainResult present();

        size_t getMaxFramesInFlight() const;
        Extent2D getExtent() const;
        inline const RenderPass& getRenderPass() const { return _renderPass; }
        inline const RenderPass* getRenderPassPtr() const { return &_renderPass; }

        inline uint32_t getImageCount() const { return _imageCount; }
        inline uint32_t getPreviousImageCount() const { return _previousImageCount; }
        inline bool imageCountChanged() const { return _previousImageCount != _imageCount; }
        inline void resetChangedImageCount() { _previousImageCount = _imageCount; }

        inline uint32_t getCurrentImageIndex() const { return _currentImageIndex; }
        inline size_t getCurrentFrame() const { return _currentFrame; }
        inline const SwapchainImpl* getImpl() const { return _pImpl; }
        inline SwapchainImpl* getImpl() { return _pImpl; }
    };
}
