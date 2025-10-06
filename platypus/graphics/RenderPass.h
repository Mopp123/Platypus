#pragma once

#include "platypus/assets/Image.h"


namespace platypus
{
    class Swapchain;

    struct RenderPassImpl;
    class RenderPass
    {
    private:
        RenderPassImpl* _pImpl = nullptr;
        bool _offscreen = false;

    public:
        RenderPass(bool offscreen);
        ~RenderPass();

        void create(
            ImageFormat colorFormat,
            ImageFormat depthFormat
        );

        void destroy();

        inline bool isOffscreenPass() const { return _offscreen; }
        inline const RenderPassImpl* getImpl() const { return _pImpl; }
    };
}
