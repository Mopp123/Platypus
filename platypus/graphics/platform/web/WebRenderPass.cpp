#include "platypus/graphics/RenderPass.hpp"

namespace platypus
{
    struct RenderPassImpl
    {
    };

    RenderPass::RenderPass(
        RenderPassType type,
        bool offscreen,
        uint32_t attachmentUsageFlags,
        uint32_t attachmentClearFlags
    ) :
        _type(type),
        _offscreen(offscreen),
        _attachmentUsageFlags(attachmentUsageFlags),
        _attachmentClearFlags(attachmentClearFlags)
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
        _colorFormat = colorFormat;
        _depthFormat = depthFormat;
    }

    void RenderPass::destroy()
    {
    }
}
