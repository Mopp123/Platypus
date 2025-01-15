#pragma once

namespace platypus
{
    struct ContextImpl;

    class Context
    {
    private:
        ContextImpl* _pImpl = nullptr;

    public:
        Context(const char* appName);
        ~Context();
    };
}
