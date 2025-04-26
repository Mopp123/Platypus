#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/platform/desktop/DesktopCommandBuffer.h"
#include "DesktopBuffers.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include <cstring>
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    void copy_buffer(const CommandPool& commandPool, VkBuffer source, VkBuffer destination, size_t size)
    {
        CommandBuffer commandBuffer = commandPool.allocCommandBuffers(
            1,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        )[0];

        commandBuffer.beginSingleUse();

        VkBufferCopy copyRegion;
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        vkCmdCopyBuffer(
            commandBuffer.getImpl()->handle,
            source,
            destination,
            1,
            &copyRegion
        );

        commandBuffer.finishSingleUse();
    }

    void copy_buffer_to_image(
        const CommandPool& commandPool,
        VkBuffer source,
        VkImage destination,
        uint32_t imageWidth,
        uint32_t imageHeight
    )
    {
        CommandBuffer commandBuffer = commandPool.allocCommandBuffers(
            1,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        )[0];

        commandBuffer.beginSingleUse();

        VkBufferImageCopy bufferImgCpy{};
        bufferImgCpy.bufferOffset = 0;
        bufferImgCpy.bufferRowLength = 0;
        bufferImgCpy.bufferImageHeight = 0;

        bufferImgCpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferImgCpy.imageSubresource.mipLevel = 0;
        bufferImgCpy.imageSubresource.baseArrayLayer = 0;
        bufferImgCpy.imageSubresource.layerCount = 1;

        bufferImgCpy.imageOffset = { 0,0,0 };
        bufferImgCpy.imageExtent = { imageWidth, imageHeight, 1 };

        vkCmdCopyBufferToImage(
            commandBuffer.getImpl()->handle,
            source,
            destination,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &bufferImgCpy
        );

        commandBuffer.finishSingleUse();
    }

    VkVertexInputRate to_vk_vertex_input_rate(VertexInputRate inputRate)
    {
        switch (inputRate)
        {
            case VERTEX_INPUT_RATE_VERTEX:   return VK_VERTEX_INPUT_RATE_VERTEX;
            case VERTEX_INPUT_RATE_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;
            default:
                Debug::log(
                    "@to_vk_vertex_input_rate invalid value for inputRate",
                    Debug::MessageType::PLATYPUS_ERROR
                );
        };
        PLATYPUS_ASSERT(false);
        return VK_VERTEX_INPUT_RATE_VERTEX;
    }

    VkFormat to_vk_format_from_shader_datatype(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:  return VK_FORMAT_R32_SFLOAT;
        case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
        case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default: return VK_FORMAT_R32_SFLOAT;
        }
    }

    VkBufferUsageFlags to_vk_buffer_usage_flags(uint32_t flags)
    {
        VkBufferUsageFlags vkFlags = 0;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT)
            vkFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT)
            vkFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            vkFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_SRC_BIT)
            vkFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT)
            vkFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        return vkFlags;
    }

    size_t get_dynamic_uniform_buffer_element_size(size_t requestSize)
    {
        size_t alignRequirement = Context::get_instance()->getMinUniformBufferOffsetAlignment();
        size_t diff = (std::max(requestSize - 1, (size_t)1)) / alignRequirement;
        return  alignRequirement * (diff + 1);
    }

    VkIndexType to_vk_index_type(size_t bufferElementSize)
    {
        switch (bufferElementSize)
        {
            case sizeof(uint16_t): return VK_INDEX_TYPE_UINT16;
            case sizeof(uint32_t): return VK_INDEX_TYPE_UINT32;
            default:
                Debug::log(
                    "@to_vk_index_type "
                    "Unsupported element size(" + std::to_string(bufferElementSize) + ") "
                    "for index buffer. Allowed element sizes for index buffers are sizes of uint16_t and uint32_t",
                    Debug::MessageType::PLATYPUS_ERROR
                );
        }
        PLATYPUS_ASSERT(false);
        return VK_INDEX_TYPE_UINT16;
    }


    VertexBufferElement::VertexBufferElement()
    {
        _pImpl = new VertexBufferElementImpl;
    }

    VertexBufferElement::VertexBufferElement(uint32_t location, ShaderDataType dataType) :
        _location(location),
        _type(dataType)
    {
        _pImpl = new VertexBufferElementImpl;
    }

    VertexBufferElement::VertexBufferElement(const VertexBufferElement& other) :
        VertexBufferElement(other._location, other._type)
    {
        _pImpl->attribDescription = other._pImpl->attribDescription;
    }

    VertexBufferElement::~VertexBufferElement()
    {
        if (_pImpl)
            delete _pImpl;
    }


    // NOTE: Not sure if copying elems goes correctly here..
    VertexBufferLayout::VertexBufferLayout(
        std::vector<VertexBufferElement> elements,
        VertexInputRate inputRate,
        uint32_t binding
    ) :
        _inputRate(inputRate)
    {
        _pImpl = new VertexBufferLayoutImpl;
        _pImpl->bindingDescription.binding = binding;
        _pImpl->bindingDescription.inputRate = to_vk_vertex_input_rate(inputRate);
        for (const VertexBufferElement& element : elements)
        {
            ShaderDataType elemType = element.getType();
            size_t elemSize = get_shader_datatype_size(elemType);

            // We dont want to touch the original element, we just copy its stuff here
            // and modify the copy's properties, so the original's state stays the same
            VertexBufferElement cpyElem = element;

            VkVertexInputAttributeDescription& attribDescRef = cpyElem._pImpl->attribDescription;
            attribDescRef.binding = binding;
            attribDescRef.location = cpyElem.getLocation();
            attribDescRef.offset = _stride; // *the "current stride", where we havent yet added this attribute's size.. that kind of IS the offset..
            attribDescRef.format = to_vk_format_from_shader_datatype(elemType);

            _stride += (uint32_t)elemSize;

            _elements.push_back(cpyElem);
        }
        _pImpl->bindingDescription.stride = _stride;
    }

    VertexBufferLayout::VertexBufferLayout(const VertexBufferLayout& other) :
        VertexBufferLayout(
            other._elements,
            other._inputRate,
            other._pImpl->bindingDescription.binding
        )
    {
    }

    VertexBufferLayout::~VertexBufferLayout()
    {
        if (_pImpl)
            delete _pImpl;
    }


    // NOTE:
    //  * "elementSize" single element's size in "data buffer"
    //  * "dataLength" number of "elements" in the "data buffer" (NOT total size)
    //  * "Data" gets just copied here! Ownership of the data doesn't get transferred here!
    //  * If usageFlags contains BUFFER_USAGE_TRANSFER_DST_BIT this will implicitly create, transfer
    //  and destroy staging buffer!
    Buffer::Buffer(
        const CommandPool& commandPool,
        void* pData,
        size_t elementSize,
        size_t dataLength,
        uint32_t usageFlags,
        BufferUpdateFrequency updateFrequency,
        bool storeHostSide
    ) :
        _dataElemSize(elementSize),
        _dataLength(dataLength),
        _bufferUsageFlags(usageFlags),
        _updateFrequency(updateFrequency)
    {
        if (storeHostSide)
        {
            _pData = calloc(_dataLength, _dataElemSize);
            memcpy(_pData, pData, getTotalSize());
        }

        bool useStaging = usageFlags & BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT;
        Buffer* pStagingBuffer = nullptr;
        if (useStaging)
        {
            pStagingBuffer = new Buffer(
                commandPool,
                pData,
                elementSize,
                dataLength,
                BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_SRC_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
                false
            );
        }

        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = getTotalSize();
        createInfo.usage = to_vk_buffer_usage_flags(_bufferUsageFlags);
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};

        if (!useStaging)
        {
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        else
        {
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        VmaAllocator vmaAllocator = Context::get_instance()->getImpl()->vmaAllocator;
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
        VkResult createResult = vmaCreateBuffer(
            vmaAllocator,
            &createInfo,
            &allocInfo,
            &buffer,
            &vmaAllocation,
            nullptr
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@Buffer::Buffer "
                "Failed to create buffer(vmaCreateBuffer)! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        _pImpl = new BufferImpl{ buffer, vmaAllocation };

        if (!useStaging)
        {
            vmaCopyMemoryToAllocation(
                vmaAllocator,
                pData,
                vmaAllocation,
                0,
                getTotalSize()
            );
        }
        else
        {
            copy_buffer(
                commandPool,
                pStagingBuffer->_pImpl->handle,
                _pImpl->handle,
                getTotalSize()
            );
            delete pStagingBuffer;
        }
    }

    Buffer::~Buffer()
    {
        if (_pData)
            free(_pData);

        if (_pImpl)
        {
            vmaDestroyBuffer(
                Context::get_instance()->getImpl()->vmaAllocator,
                _pImpl->handle,
                _pImpl->vmaAllocation
            );
            delete _pImpl;
        }
    }

    void Buffer::updateDevice(void* pData, size_t dataSize, size_t offset)
    {
        if (!_hostSideUpdated)
        {
            Debug::log(
                "@Buffer::updateDevice "
                "Host side buffer exists but wasn't updated!",
                Debug::MessageType::PLATYPUS_WARNING
            );
        }

        if (!validateUpdate(pData, dataSize, offset))
        {
            Debug::log(
                "@Buffer::updateDevice "
                "Failed to update buffer!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        vmaCopyMemoryToAllocation(
            Context::get_instance()->getImpl()->vmaAllocator,
            pData,
            _pImpl->vmaAllocation,
            offset,
            dataSize
        );
        _hostSideUpdated = false;
    }
}
