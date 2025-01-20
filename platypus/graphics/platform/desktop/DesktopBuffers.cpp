#include "platypus/graphics/Buffers.h"
#include "DesktopBuffers.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static VkVertexInputRate to_vk_vertex_input_rate(VertexInputRate inputRate)
    {
        switch (inputRate)
        {
            case VERTEX_INPUT_RATE_VERTEX: return VK_VERTEX_INPUT_RATE_VERTEX;
            case VERTEX_INPUT_RATE_INSTANCE: return VK_VERTEX_INPUT_RATE_INSTANCE;
            default:
                Debug::log("@to_vk_vertex_input_rate invalid value for inputRate", Debug::MessageType::PLATYPUS_ERROR);
                PLATYPUS_ASSERT(false);
                return VK_VERTEX_INPUT_RATE_VERTEX;
        };
    }

    VkFormat to_vk_format_from_shader_datatype(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
        case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
        case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default: return VK_FORMAT_R32_SFLOAT;
        }
    }

    size_t get_shader_datatype_size(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float: return sizeof(float);
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
        case ShaderDataType::Float: return 1;
        case ShaderDataType::Float2: return 2;
        case ShaderDataType::Float3: return 3;
        case ShaderDataType::Float4: return 4;
        default: return 0;
        }
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


    Buffer::Buffer(
        void* data,
        size_t elementSize,
        size_t dataLength,
        uint32_t bufferUsageFlags,
        BufferUpdateFrequency bufferUpdateFrequency,
        bool saveDataHostSide
    )
    {
    }

    Buffer::~Buffer()
    {
    }
}
