#pragma once

#include "platypus/assets/Texture.h"
#include <cstdint>


namespace platypus
{
    struct FramebufferImpl
    {
        uint32_t id = 0;
        uint32_t depthRenderBuffer = 0;


        // If attempting to use a depth attachment which is already used by another framebuffer,
        // create another depth attachment for this framebuffer and blit the intended depth
        // attachment to this at the beginning of render pass.
        //
        // This had to be done because couldn't use same depth attachment while sampling it in
        // shader in OpenGL ES 3 (like how dealing with opaque and transparent passes using Vulkan)
        //
        // NOTE: May need to do the same thing for color attachments in the future?

        // *Need this since the actual asset may get destroyed before Framebuffer's destructor in
        // which we'll erase the used depth attachment's ID from the s_boundFramebufferAttachments
        ID_t originalDepthAttachmentAssetID = NULL_ID;
        uint32_t copySourceID = 0;
        Texture* pCopyDepthAttachment = nullptr;
    };
}
