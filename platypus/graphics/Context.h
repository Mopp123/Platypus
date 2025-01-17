#pragma once

#include "platypus/core/Window.h"


namespace platypus
{
    struct ContextImpl;
    class Swapchain;

    class Context
    {
    private:
        friend class Swapchain;
        ContextImpl* _pImpl = nullptr;

    public:
        Context(const char* appName, Window* pWindow);
        ~Context();
    };
}
