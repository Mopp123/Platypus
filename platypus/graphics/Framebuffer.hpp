#pragma once

namespace platypus
{
    struct FramebufferImpl;
    class Framebuffer
    {
    private:
        FramebufferImpl* _pFramebufferImpl = nullptr;

    public:
        Framebuffer();
        ~Framebuffer();

        FramebufferImpl* getImpl();
    };
}
