#include "platypus/graphics/CommandBuffer.h"
#include "DesktopCommandBuffer.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include "platypus/core/Debug.h"
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>


namespace platypus
{
    CommandBuffer::CommandBuffer(CommandPool* pPool) :
        _pPool(pPool)
    {
        _pImpl = new CommandBufferImpl;
    }

    CommandBuffer::CommandBuffer(const CommandBuffer& other) :
        _pPool(other._pPool)
    {
        _pImpl = new CommandBufferImpl;
        _pImpl->handle = other._pImpl->handle;
    }

    CommandBuffer::~CommandBuffer()
    {
        if (_pImpl)
            delete _pImpl;
    }

    void CommandBuffer::free()
    {
        vkFreeCommandBuffers(
            Context::get_pimpl()->device,
            _pPool->_pImpl->handle,
            1,
            &_pImpl->handle
        );
    }


    CommandPool::CommandPool()
    {
        const ContextImpl * const pContextImpl = Context::get_pimpl();

        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.queueFamilyIndex = pContextImpl->deviceQueueFamilyIndices.graphicsFamily;
        // Possible flags:
        // * VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // * VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        // These flags should be fine for now...
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool commandPool;
        VkResult createResult = vkCreateCommandPool(
            pContextImpl->device,
            &createInfo,
            nullptr,
            &commandPool
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@CommandPool::CommandPool "
                "Failed to create command pool! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        _pImpl = new CommandPoolImpl;
        _pImpl->handle = commandPool;

        Debug::log("Command pool created");
    }

    CommandPool::~CommandPool()
    {
        // NOTE: All command buffers allocated from this pool should be freed at this point?
        vkDestroyCommandPool(
            Context::get_pimpl()->device,
            _pImpl->handle,
            nullptr
        );
        delete _pImpl;
    }

    std::vector<CommandBuffer> CommandPool::allocCommandBuffers(
        uint32_t count,
        CommandBufferLevel level
    )
    {
        std::vector<VkCommandBuffer> bufferHandles(count);
        std::vector<CommandBuffer> buffers;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _pImpl->handle;
        allocInfo.level = level == CommandBufferLevel::PRIMARY_COMMAND_BUFFER ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = count;

        VkResult allocResult = vkAllocateCommandBuffers(
            Context::get_pimpl()->device,
            &allocInfo,
            bufferHandles.data()
        );
        if (allocResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(allocResult));
            Debug::log(
                "@CommandPool::allocCommandBuffers "
                "Failed to allocate " + std::to_string(count) + " command buffers! "
                "VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            CommandBuffer buffer(this);
            buffer._pImpl->handle = bufferHandles[i];
            buffers.push_back(buffer);
        }

        return buffers;
    }
}
