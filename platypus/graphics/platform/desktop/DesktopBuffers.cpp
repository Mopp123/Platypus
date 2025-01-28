#include "platypus/graphics/Buffers.h"
#include "DesktopBuffers.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include "platypus/core/Debug.h"
#include <cstring>
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
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

    // NOTE: This is ment to convert a SINGLE flag into a SINGLE VkBufferUsageFlag
    VkBufferUsageFlags to_vk_buffer_usage_flags(uint32_t flags)
    {
        VkBufferUsageFlags vkFlags = 0;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT)
            vkFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT)
            vkFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (flags & BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            vkFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        return vkFlags;
    }

    size_t get_shader_datatype_size(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:  return sizeof(float);
        case ShaderDataType::Float2: return sizeof(float) * 2;
        case ShaderDataType::Float3: return sizeof(float) * 3;
        case ShaderDataType::Float4: return sizeof(float) * 4;
        default: return 0;
        }
    }

    uint32_t get_shader_datatype_component_count(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:  return 1;
        case ShaderDataType::Float2: return 2;
        case ShaderDataType::Float3: return 3;
        case ShaderDataType::Float4: return 4;
        default: return 0;
        }
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
    )
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


    // TODO: Optimizations:
    // * staging buffers
    Buffer::Buffer(
        void* pData,
        size_t elementSize,
        size_t dataLength,
        uint32_t usageFlags,
        BufferUpdateFrequency updateFrequency,
        bool saveDataHostSide
    ) :
        _dataElemSize(elementSize),
        _dataLength(dataLength),
        _bufferUsageFlags(usageFlags),
        _updateFrequency(updateFrequency)
    {
        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = getTotalSize();
        createInfo.usage = to_vk_buffer_usage_flags(_bufferUsageFlags);
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

        // ATM JUST TESTING WRITING DIRECTLY
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
        VkResult createResult = vmaCreateBuffer(
            Context::get_pimpl()->vmaAllocator,
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

        // ATM JUST TESTING WRITING DIRECTLY!
        _pData = calloc(dataLength, elementSize);
        memcpy(_pData, pData, elementSize * dataLength);

        VmaAllocationInfo allocatedInfo;
        vmaGetAllocationInfo(
            Context::get_pimpl()->vmaAllocator,
            vmaAllocation,
            &allocatedInfo
        );
        memcpy(allocatedInfo.pMappedData, _pData, getTotalSize());

        _pImpl = new BufferImpl{ buffer, vmaAllocation };
    }

    Buffer::~Buffer()
    {
        if (_pImpl)
        {
            vmaDestroyBuffer(
                Context::get_pimpl()->vmaAllocator,
                _pImpl->handle,
                _pImpl->vmaAllocation
            );
        }
        if (_pData)
            free(_pData);
    }
}
