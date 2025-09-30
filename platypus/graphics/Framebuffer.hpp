#pragma once

#include "RenderPass.h"
#include "platypus/assets/Texture.h"

namespace platypus
{
    struct FramebufferImpl;
    class Framebuffer
    {
    private:
        FramebufferImpl* _pImpl = nullptr;
        uint32_t _width = 0;
        uint32_t _height = 0;
        std::vector<Texture*> _attachments;

    public:
        Framebuffer(
            const RenderPass& renderPass,
            const std::vector<Texture*>& attachments,
            uint32_t width,
            uint32_t height
        );
        ~Framebuffer();

        inline FramebufferImpl* getImpl() { return _pImpl; }
    };
}
