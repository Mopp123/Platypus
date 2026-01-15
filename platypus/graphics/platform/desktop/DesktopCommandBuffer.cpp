#include "platypus/graphics/CommandBuffer.hpp"
#include "DesktopCommandBuffer.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/platform/desktop/DesktopDevice.hpp"
#include "DesktopContext.hpp"
#include "DesktopRenderPass.hpp"
#include "platypus/core/Debug.hpp"
#include <vulkan/vk_enum_string_helper.h>


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
            Device::get_impl()->device,
            _pPool->_pImpl->handle,
            1,
            &_pImpl->handle
        );
        _pImpl->handle = VK_NULL_HANDLE;
    }

    void CommandBuffer::begin(const RenderPass* pRenderPass)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VkCommandBufferInheritanceInfo inheritanceInfo{};
        if (_level == CommandBufferLevel::SECONDARY_COMMAND_BUFFER)
        {
            PLATYPUS_ASSERT(pRenderPass);
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            inheritanceInfo.renderPass = pRenderPass->getImpl()->handle;
            inheritanceInfo.subpass = 0;
            beginInfo.pInheritanceInfo = &inheritanceInfo;
        }
        VkResult beginResult = vkBeginCommandBuffer(_pImpl->handle, &beginInfo);
        if (beginResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(beginResult));
            const std::string typeStr = _level == CommandBufferLevel::PRIMARY_COMMAND_BUFFER ? "primary" : "secondary";
            Debug::log(
                "@CommandBuffer::begin "
                "Failed to begin " + typeStr + " command buffer! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void CommandBuffer::end()
    {
        if (_pImpl->pipelineLayout != VK_NULL_HANDLE)
            _pImpl->pipelineLayout = VK_NULL_HANDLE;
        VkResult endResult = vkEndCommandBuffer(_pImpl->handle);
        if (endResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(endResult));
            const std::string typeStr = _level == CommandBufferLevel::PRIMARY_COMMAND_BUFFER ? "primary" : "secondary";
            Debug::log(
                "@CommandBuffer::end "
                "Failed to end " + typeStr + " command buffer! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void CommandBuffer::beginSingleUse()
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkResult beginResult = vkBeginCommandBuffer(_pImpl->handle, &beginInfo);
        if (beginResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(beginResult));
            Debug::log(
                "@CommandBuffer::beginSingleUse "
                "Failed to begin single use command buffer! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
    }

    void CommandBuffer::finishSingleUse()
    {
        VkCommandBuffer handle = _pImpl->handle;
        VkResult endResult = vkEndCommandBuffer(handle);
        if (endResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(endResult));
            Debug::log(
                "@CommandBuffer::finishSingleUse "
                "Failed to end single use command buffer! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &handle;

        VkQueue graphicsQueue = Device::get_impl()->graphicsQueue;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        free();
    }


    CommandPool::CommandPool()
    {
        DeviceImpl* pDeviceImpl = Device::get_impl();
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.queueFamilyIndex = pDeviceImpl->physicalDevice.queueProperties.graphicsFamilyIndex;
        // Possible flags:
        // * VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often (may change memory allocation behavior)
        // * VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        // These flags should be fine for now...
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool commandPool;
        VkResult createResult = vkCreateCommandPool(
            pDeviceImpl->device,
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
            Device::get_impl()->device,
            _pImpl->handle,
            nullptr
        );
        delete _pImpl;
    }

    std::vector<CommandBuffer> CommandPool::allocCommandBuffers(
        uint32_t count,
        CommandBufferLevel level
    ) const
    {
        std::vector<VkCommandBuffer> bufferHandles(count);
        std::vector<CommandBuffer> buffers;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _pImpl->handle;
        allocInfo.level = level == CommandBufferLevel::PRIMARY_COMMAND_BUFFER ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = count;

        VkResult allocResult = vkAllocateCommandBuffers(
            Device::get_impl()->device,
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
            CommandBuffer buffer(this, level);
            buffer._pImpl->handle = bufferHandles[i];
            buffers.push_back(buffer);
        }

        return buffers;
    }
}
