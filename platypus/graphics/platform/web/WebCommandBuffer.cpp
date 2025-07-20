#include "platypus/graphics/CommandBuffer.h"
#include "WebCommandBuffer.h"
#include "WebContext.hpp"
#include <GL/glew.h>


namespace platypus
{
    CommandBuffer::CommandBuffer(const CommandPool* pPool, CommandBufferLevel level) :
        _pPool(pPool),
        _level(level)
    {
        _pImpl = new CommandBufferImpl;
    }

    CommandBuffer::CommandBuffer(const CommandBuffer& other) :
        _pPool(other._pPool),
        _level(other._level)
    {
        _pImpl = new CommandBufferImpl;
    }

    CommandBuffer::~CommandBuffer()
    {
        if (_pImpl)
            delete _pImpl;
    }

    void CommandBuffer::free()
    {
    }

    void CommandBuffer::begin(const RenderPass& renderPass)
    {
    }

    void CommandBuffer::end()
    {
        // unbind all
        GL_FUNC(glActiveTexture(GL_TEXTURE0));
        GL_FUNC(glBindTexture(GL_TEXTURE_2D, 0));
        GL_FUNC(glBindBuffer(GL_ARRAY_BUFFER, 0));
        GL_FUNC(glBindVertexArray(0));

        // Make sure this cmd buf is unable to touch any pipeline until calling bindPipeline() again
        _pImpl->pPipelineImpl = nullptr;
        _pImpl->drawIndexedType = IndexType::INDEX_TYPE_NONE;
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
