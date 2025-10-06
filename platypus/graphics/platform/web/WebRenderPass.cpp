#include "platypus/graphics/RenderPass.h"

namespace platypus
{
    struct RenderPassImpl
    {
    };

    RenderPass::RenderPass(bool offscreen) :
        _offscreen(offscreen)
    {
    }

    RenderPass::~RenderPass()
    {
    }

    void RenderPass::create(
        ImageFormat colorFormat,
        ImageFormat depthFormat
    )
    {
    }

    void RenderPass::destroy()
    {
    }
}
