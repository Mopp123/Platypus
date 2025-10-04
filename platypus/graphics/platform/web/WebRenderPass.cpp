#include "platypus/graphics/RenderPass.h"

namespace platypus
{
    struct RenderPassImpl
    {
    };

    RenderPass::RenderPass()
    {
    }

    RenderPass::~RenderPass()
    {
    }

    void RenderPass::create(
        ImageFormat colorFormat,
        ImageFormat depthFormat,
        bool offscreenTarget
    )
    {
    }

    void RenderPass::destroy()
    {
    }
}
