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

        inline const FramebufferImpl* getImpl() const { return _pImpl; }
        inline FramebufferImpl* getImpl() { return _pImpl; }

        inline uint32_t getWidth() const { return _width; }
        inline uint32_t getHeight() const { return _height; }

        inline std::vector<Texture*>& getAttachments() { return _attachments; }
    };
}
