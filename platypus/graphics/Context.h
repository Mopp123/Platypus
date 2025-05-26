#pragma once

#include "platypus/core/Window.h"

// Don't care to mess with adapting to different systems having
// different max push constants sizes, so we'll cap it to
// quaranteed minimum
#define PLATYPUS_MAX_PUSH_CONSTANTS_SIZE 128


namespace platypus
{
    class Swapchain;
    class Shader;
    class Buffer;
    class CommandBuffer;

    struct ContextImpl;
    class Context
    {
    private:
        friend class Swapchain;
        friend class Shader;
        friend class Buffer;

        static Context* s_pInstance;
        ContextImpl* _pImpl = nullptr;

        size_t _minUniformBufferOffsetAlignment = 1;

    public:
        Context(const char* appName, Window* pWindow);
        ~Context();

        // TODO: Completely separate class for "Device", handling Device specific stuff like this
        void submitPrimaryCommandBuffer(Swapchain& swapchain, const CommandBuffer& cmdBuf, size_t frame);

        // *On vulkan side we need to wait for device operations to finish before freeing resources used
        // by these operations
        void waitForOperations();

        // *On vulkan, required to re query swapchain support details to recreate swapchain
        void handleWindowResize();

        static Context* get_instance();

        // Required for vulkan's descriptor sets using dynamic offsets of uniform buffers.
        // For OpenGL this should be always 1
        inline size_t getMinUniformBufferOffsetAlignment() const { return _minUniformBufferOffsetAlignment; }
        inline const ContextImpl * const getImpl() const { return _pImpl; }
    };
}
