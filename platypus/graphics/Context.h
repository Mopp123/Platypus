#pragma once

#include "platypus/core/Window.h"


namespace platypus
{
    struct ContextImpl;

    class Context
    {
    private:
        ContextImpl* _pImpl = nullptr;

    public:
        Context(const char* appName, Window* pWindow);
        ~Context();
    };
}
