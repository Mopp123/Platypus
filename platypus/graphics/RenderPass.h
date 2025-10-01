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

    public:
        RenderPass();
        ~RenderPass();

        void create(
            ImageFormat colorFormat,
            ImageFormat depthFormat,
            bool offscreenTarget
        );

        void destroy();

        inline const RenderPassImpl* getImpl() const { return _pImpl; }
    };
}
