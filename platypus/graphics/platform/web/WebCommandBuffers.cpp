#include "platypus/graphics/CommandBuffer.h"


namespace platypus
{
    struct CommandBufferImpl
    {
    };

    CommandBuffer::CommandBuffer(const CommandPool* pPool, CommandBufferLevel level) :
        _pPool(pPool),
        _level(level)
    {
    }

    CommandBuffer::CommandBuffer(const CommandBuffer& other) :
        _pPool(other._pPool),
        _level(other._level)
    {
    }

    CommandBuffer::~CommandBuffer()
    {
    }

    void CommandBuffer::free()
    {
    }

    void CommandBuffer::begin(const RenderPass& renderPass)
    {
    }

    void CommandBuffer::end()
    {
    }

    void CommandBuffer::beginSingleUse()
    {
    }

    void CommandBuffer::finishSingleUse()
    {
    }


    struct CommandPoolImpl
    {
    };

    CommandPool::CommandPool()
    {
    }

    CommandPool::~CommandPool()
    {
    }

    std::vector<CommandBuffer> CommandPool::allocCommandBuffers(
        uint32_t count,
        CommandBufferLevel level
    ) const
    {
        std::vector<CommandBuffer> commandBuffers;
        for (uint32_t i = 0; i < count; ++i)
        {
            commandBuffers.push_back({ this, level });
        }

        return commandBuffers;
    }
}
