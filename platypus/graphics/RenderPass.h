#pragma once


namespace platypus
{
    struct RenderPassImpl;

    class Swapchain;
    class Pipeline;

    class RenderPass
    {
    private:
        friend class Swapchain;
        friend class Pipeline;
        RenderPassImpl* _pImpl = nullptr;

    public:
        RenderPass();
        ~RenderPass();

        void create(const Swapchain& swapchain);
        void destroy();
    };
}
