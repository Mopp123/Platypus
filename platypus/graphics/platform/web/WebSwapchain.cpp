#include "platypus/graphics/Swapchain.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    struct SwapchainImpl
    {
        Extent2D extent;
    };


    Swapchain::Swapchain(const Window& window, bool createDepthAttachment) :
        _renderPass(
            RenderPassType::SCREEN_PASS,
            false,
            createDepthAttachment ? RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_COLOR_DISCRETE | RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_DEPTH_DISCRETE : RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_COLOR_DISCRETE,
            createDepthAttachment ? RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_COLOR | RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_DEPTH : RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_COLOR
        ),
        _createDepthAttachment(createDepthAttachment)
    {
        _pImpl = new SwapchainImpl;
        create(window);
    }

    Swapchain::~Swapchain()
    {
        destroy();
        if (_pImpl)
            delete _pImpl;
    }

    // TODO: Window surface (just a 2D quad)
    void Swapchain::create(const Window& window)
    {
        _renderPass.create(ImageFormat::R8G8B8A8_UNORM, _createDepthAttachment ? ImageFormat::D32_SFLOAT : ImageFormat::NONE);

        int width = 0;
        int height = 0;
        window.getSurfaceExtent(&width, &height);
        _pImpl->extent = { (uint32_t)width, (uint32_t)height };
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
        if (_pImpl)
        {
            return _pImpl->extent;
        }
        Debug::log(
            "@Swapchain::getExtent "
            "Swapchain's pImpl was nullptr",
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
    }
}
