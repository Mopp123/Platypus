#pragma once


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

        void create(const Swapchain& swapchain);
        void destroy();

        inline const RenderPassImpl* getImpl() const { return _pImpl; }
    };
}
