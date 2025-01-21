#pragma once


namespace platypus
{
    struct RenderPassImpl;

    class Swapchain;

    class RenderPass
    {
    private:
        friend class Swapchain;
        RenderPassImpl* _pImpl = nullptr;

    public:
        RenderPass();
        ~RenderPass();

        void create(const Swapchain& swapchain);
        void destroy();
    };
}
