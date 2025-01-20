#pragma once

#include "platypus/core/Window.h"


namespace platypus
{
    struct ContextImpl;
    class Swapchain;
    class Shader;
    class Buffer;

    class Context
    {
    private:
        friend class Swapchain;
        friend class Shader;
        friend class Buffer;
        static ContextImpl* s_pImpl;

    public:
        Context(const char* appName, Window* pWindow);
        ~Context();

        static const ContextImpl * const get_pimpl();
    };
}
