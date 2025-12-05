#include "RenderPass.hpp"
#include "platypus/core/Debug.h"
#include "Framebuffer.hpp"
#include "platypus/core/Application.h"


namespace platypus
{

    RenderPassInstance::RenderPassInstance(
        const RenderPass& renderPass,
        uint32_t framebufferWidth,
        uint32_t framebufferHeight,
        bool useWindowDimensions
    ) :
        _renderPassRef(renderPass),
        _framebufferWidth(framebufferWidth),
        _framebufferHeight(framebufferHeight),
        _useWindowDimensions(useWindowDimensions)
    {
        if (_useWindowDimensions)
            matchWindowDimensions();
    }

    RenderPassInstance::~RenderPassInstance()
    {
        destroyFramebuffers();
    }

    void RenderPassInstance::destroyFramebuffers()
    {
        for (Framebuffer* pFramebuffer : _framebuffers)
            delete pFramebuffer;

        _framebuffers.clear();
    }

    void RenderPassInstance::createFramebuffers(
        const std::vector<Texture*>& colorAttachments,
        Texture* pDepthAttachment,
        size_t count
    )
    {
        if (_useWindowDimensions)
            matchWindowDimensions();

        for (size_t i = 0; i < count; ++i)
        {
            _framebuffers.push_back(
                new Framebuffer(
                    _renderPassRef,
                    colorAttachments,
                    pDepthAttachment,
                    _framebufferWidth,
                    _framebufferHeight
                )
            );
        }
    }

    Framebuffer* RenderPassInstance::getFramebuffer(size_t frame) const
    {
        if (_framebuffers.empty())
        {
            Debug::log(
                "@RenderPassInstance::getFramebuffer "
                "No framebuffers exist.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        const size_t framebufferCount = _framebuffers.size();
        if (framebufferCount == 1)
            return _framebuffers[0];

        if (frame >= framebufferCount)
        {
            Debug::log(
                "@RenderPassInstance::getFramebuffer "
                "Requested framebuffer at index: " + std::to_string(frame) + " "
                "framebuffer count was: " + std::to_string(framebufferCount) + ". "
                "Framebuffer count has to be either 1 or "
                "match the count of max frames in flight!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }

        return _framebuffers[frame];
    }

    void RenderPassInstance::matchWindowDimensions()
    {
        Extent2D swapchainExtent = Application::get_instance()->getMasterRenderer()->getSwapchain().getExtent();
        _framebufferWidth = swapchainExtent.width;
        _framebufferHeight = swapchainExtent.height;
    }
}
