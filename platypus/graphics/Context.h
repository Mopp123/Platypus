#pragma once

#include "platypus/core/Window.h"


namespace platypus
{
    struct ContextImpl;
    class Swapchain;
    class Shader;

    class Context
    {
    private:
        friend class Swapchain;
        friend class Shader;
        static ContextImpl* s_pImpl;

    public:
        Context(const char* appName, Window* pWindow);
        ~Context();

        static const ContextImpl * const get_pimpl();
    };
}
