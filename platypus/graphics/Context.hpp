#pragma once

#include "platypus/core/Window.hpp"

// Don't care to mess with adapting to different systems having
// different max push constants sizes, so we'll cap it to max
// quaranteed minimum
#define PLATYPUS_MAX_PUSH_CONSTANTS_SIZE 128

namespace platypus
{
    struct ContextImpl;
    class Shader;
    class Buffer;
    class CommandBuffer;

    class Context
    {
    private:
        friend class Shader;
        friend class Buffer;

        static Window* s_pWindow;
        static ContextImpl* s_pImpl;

    public:
        static void create(const char* appName, Window* pWindow);
        static void destroy();

        static ContextImpl* get_impl();
    };
}
