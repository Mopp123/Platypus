#pragma once

#include "platypus/assets/Image.h"
#include <vector>
#include <string>


namespace platypus
{
    enum class RenderPassType
    {
        SCENE_PASS,
        SHADOW_PASS
    };

    static inline std::string render_pass_type_to_string(RenderPassType type)
    {
        switch (type)
        {
            case RenderPassType::SCENE_PASS:    return "SCENE_PASS";
            case RenderPassType::SHADOW_PASS:   return "SHADOW_PASS";
            default:                            return "<Invalid RenderPassType>";
        }
    }


    class Swapchain;

    struct RenderPassImpl;
    class RenderPass
    {
    private:
        RenderPassImpl* _pImpl = nullptr;
        RenderPassType _type;
        bool _offscreen = false;

    public:
        RenderPass(RenderPassType type, bool offscreen);
        ~RenderPass();

        void create(
            ImageFormat colorFormat,
            ImageFormat depthFormat
        );

        void destroy();

        inline RenderPassType getType() const { return _type; }
        inline bool isOffscreenPass() const { return _offscreen; }
        inline const RenderPassImpl* getImpl() const { return _pImpl; }
    };


    class Framebuffer;
    class Texture;
    class RenderPassInstance
    {
    private:
        const RenderPass& _renderPassRef;
        uint32_t _framebufferWidth = 0;
        uint32_t _framebufferHeight = 0;
        bool _useWindowDimensions = false;
        std::vector<Framebuffer*> _framebuffers;

    public:
        RenderPassInstance(
            const RenderPass& renderPass,
            uint32_t framebufferWidth,
            uint32_t framebufferHeight,
            bool useWindowDimensions
        );
        ~RenderPassInstance();

        void destroyFramebuffers();
        void createFramebuffers(
            const std::vector<Texture*>& colorAttachments,
            Texture* pDepthAttachment,
            size_t count
        );

        inline const RenderPass& getRenderPass() const { return _renderPassRef; }
        Framebuffer* getFramebuffer(size_t frame) const;

    private:
        void matchWindowDimensions();
    };
}
