#pragma once

#include <vector>
#include <cstdint>


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
        CommandPool* _pPool = nullptr;
        CommandBufferImpl* _pImpl = nullptr;

    public:
        CommandBuffer() = default;
        CommandBuffer(CommandPool* pPool);
        CommandBuffer(const CommandBuffer& other);
        ~CommandBuffer();

        void free();
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
        );
    };
}
