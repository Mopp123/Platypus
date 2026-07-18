#pragma once

#include "RenderPass.hpp"
#include "Framebuffer.hpp"
#include "platypus/assets/Texture.hpp"


namespace platypus
{
    class RenderPassInstance
    {
    private:
        const RenderPass& _renderPassRef;
        uint32_t _framebufferWidth = 0;
        uint32_t _framebufferHeight = 0;
        const TextureSampler* _pTextureSampler;
        bool _useWindowDimensions = false;
        Texture* _pColorAttachment = nullptr;
        Texture* _pDepthAttachment = nullptr;
        std::vector<Framebuffer*> _framebuffers;

    public:
        RenderPassInstance(
            const RenderPass& renderPass,
            uint32_t framebufferWidth,
            uint32_t framebufferHeight,
            const TextureSampler* pTextureSampler,
            bool useWindowDimensions
        );
        ~RenderPassInstance();

        void create();
        void destroy();

        inline const RenderPass& getRenderPass() const { return _renderPassRef; }
        Framebuffer* getFramebuffer(size_t frame) const;

    private:
        void matchWindowDimensions();
    };
}
