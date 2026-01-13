#pragma once

#include "platypus/core/Window.hpp"
#include "RenderPass.hpp"
#include "Framebuffer.hpp"


namespace platypus
{
    class Texture;

    enum class SwapchainResult
    {
        ERROR,
        RESIZE_REQUIRED,
        SUCCESS
    };


    struct SwapchainImpl;
    class Swapchain
    {
    private:
        friend class RenderPass;
        SwapchainImpl* _pImpl = nullptr;

        RenderPass _renderPass;
        std::vector<Image*> _colorImages;
        std::vector<Texture*> _colorTextures;

        Image* _pDepthImage;
        Texture* _pDepthTexture;
        std::vector<Framebuffer*> _framebuffers;

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

        inline const std::vector<Image*> getColorImages() const { return _colorImages; }
        inline const std::vector<Texture*> getColorTextures() const { return _colorTextures; }
        inline const Image* getDepthImage() const { return _pDepthImage; }
        inline const Texture* getDepthTexture() const { return _pDepthTexture; }
        inline const std::vector<Framebuffer*>& getFramebuffers() const { return _framebuffers; }
        inline std::vector<Framebuffer*>& getFramebuffers() { return _framebuffers; }
        inline const Framebuffer* getCurrentFramebuffer() const
        {
            if (_framebuffers.empty())
                return nullptr;

            return _framebuffers[_currentImageIndex];
        }
        inline Framebuffer* getCurrentFramebuffer()
        {
            if (_framebuffers.empty())
                return nullptr;

            return _framebuffers[_currentImageIndex];
        }

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
