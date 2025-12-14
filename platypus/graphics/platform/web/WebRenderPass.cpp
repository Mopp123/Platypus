#include "platypus/graphics/RenderPass.hpp"

namespace platypus
{
    struct RenderPassImpl
    {
    };

    RenderPass::RenderPass(RenderPassType type, bool offscreen) :
        _type(type),
        _offscreen(offscreen)
    {
    }

    RenderPass::~RenderPass()
    {
    }

    void RenderPass::create(
        ImageFormat colorFormat,
        ImageFormat depthFormat,
        bool clearColorAttachment,
        bool clearDepthAttachment,
        bool continueAttachmentUsage
    )
    {
        _colorFormat = colorFormat;
        _depthFormat = depthFormat;
    }

    void RenderPass::destroy()
    {
    }
}
