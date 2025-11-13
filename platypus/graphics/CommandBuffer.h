#pragma once

#include <vector>
#include <cstdint>
#include "RenderPass.h"


namespace  platypus
{
    enum class CommandBufferLevel
    {
        PRIMARY_COMMAND_BUFFER,
        SECONDARY_COMMAND_BUFFER
    };

    class CommandPool;

    struct CommandBufferImpl;
    class CommandBuffer
    {
    private:
        friend class CommandPool;
        const CommandPool* _pPool = nullptr;
        CommandBufferImpl* _pImpl = nullptr;
        CommandBufferLevel _level;

    public:
        CommandBuffer() = default;
        CommandBuffer(const CommandPool* pPool, CommandBufferLevel level);
        CommandBuffer(const CommandBuffer& other);
        ~CommandBuffer();

        void free();

        // pRenderPass can also be nullptr for primary command buffers
        void begin(const RenderPass* pRenderPass);
        void end();

        void beginSingleUse();
        void finishSingleUse();

        // Need to access this in RenderCommand implementations which I want to keep just as functions,
        // so can't just declare a friend for CommandBuffer
        inline const CommandBufferImpl* getImpl() const { return _pImpl; }
        inline CommandBufferImpl* getImpl() { return _pImpl; }
    };


    struct CommandPoolImpl;
    class CommandPool
    {
    private:
        friend class CommandBuffer;
        CommandPoolImpl* _pImpl = nullptr;

    public:
        CommandPool();
        ~CommandPool();

        std::vector<CommandBuffer> allocCommandBuffers(
            uint32_t count,
            CommandBufferLevel level
        ) const;
    };
}
