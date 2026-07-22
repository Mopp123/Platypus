#include "Buffers.hpp"
#include "Device.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/Common.h"
#include <cstring>


// The reason for this file:
//  -> Needed to have some common funcs defined which applies to all implementations

namespace platypus
{
    bool VertexBufferElement::operator==(const VertexBufferElement& other) const
    {
        return _location == other._location &&
            _dataType == other._dataType &&
            _attribType == other._attribType;
    }

    bool VertexBufferElement::operator!=(const VertexBufferElement& other) const
    {
        return _location != other._location ||
            _dataType != other._dataType ||
            _attribType != other._attribType;
    }


    /*
        Serialized format:
            uint32_t _location
            ShaderDataType _dataType
            VertexAttributeType _attribType
    */
    std::vector<char> VertexBufferElement::serialize() const
    {
        const size_t serializedSize = get_serialized_size();
        std::vector<char> serializedData(serializedSize);
        memset(serializedData.data(), 0, serializedSize);

        memcpy(serializedData.data(), &_location, sizeof(uint32_t));
        size_t pos = sizeof(uint32_t);

        memcpy(serializedData.data() + pos, &_dataType, sizeof(ShaderDataType));
        pos += sizeof(ShaderDataType);

        memcpy(serializedData.data() + pos, &_attribType, sizeof(VertexAttributeType));
        pos += sizeof(VertexAttributeType);

        PLATYPUS_ASSERT(pos == serializedSize);

        return serializedData;
    }

    VertexBufferElement VertexBufferElement::deserialize(
        const std::vector<char>& data,
        size_t offset
    )
    {
        const size_t serializedSize = get_serialized_size();
        PLATYPUS_ASSERT(offset + serializedSize <= data.size());

        uint32_t location;
        ShaderDataType dataType;
        VertexAttributeType attribType;

        memcpy(&location, data.data() + offset, sizeof(uint32_t));
        size_t pos = offset + sizeof(uint32_t);

        memcpy(&dataType, data.data() + pos, sizeof(ShaderDataType));
        pos += sizeof(ShaderDataType);

        memcpy(&attribType, data.data() + pos, sizeof(VertexAttributeType));
        pos += sizeof(VertexAttributeType);

        return {
            location,
            dataType,
            attribType
        };
    }

    size_t VertexBufferElement::get_serialized_size()
    {
        return sizeof(uint32_t) + sizeof(ShaderDataType) + sizeof(VertexAttributeType);
    }


    bool VertexBufferLayout::operator==(const VertexBufferLayout& other) const
    {
        return _elements == other._elements && _inputRate == other._inputRate && _stride == other._stride;
    }

    bool VertexBufferLayout::operator!=(const VertexBufferLayout& other) const
    {
        return _elements != other._elements || _inputRate != other._inputRate || _stride != other._stride;
    }


    std::string VertexBufferLayout::toString()
    {
        std::string result = "Binding: " + std::to_string(_binding) + "\n";
        result += "InputRate: " + std::to_string(_inputRate) + "\n";
        result += "Stride: " + std::to_string(_stride) + "\n";
        result += "Elements(" + std::to_string(_elements.size()) + "):\n";
        for (size_t i = 0; i < _elements.size(); ++i)
        {
            const VertexBufferElement& element = _elements[i];
            result += "    [" + std::to_string(i) + "]: {\n";

            result += "        Location: " + std::to_string(element.getLocation()) + "\n";
            result += "        DataType: " + shader_datatype_to_string(element.getDataType()) + "\n";
            result += "        AttributeType: " + std::to_string(static_cast<uint32_t>(element.getAttribType())) + "\n";

            result += "    }\n";
        }
        return result;
    }


    /*
        Serialized format:
            VertexInputRate inputRate
            uint32_t binding
            uint32_t elementCount
            VertexBufferElement pElements[elementCount]
    */
    std::vector<char> VertexBufferLayout::serialize() const
    {
        const size_t elementCount = _elements.size();
        const size_t serializedElementSize = VertexBufferElement::get_serialized_size();
        const size_t elementsSize = serializedElementSize * elementCount;

        // NOTE:
        //  *Not serializing the stride here atm, since we figure it out with the elements
        //  in the constructor...
        //  ->Might be required for something in the future tho?
        const uint32_t elementCountU32 = static_cast<const uint32_t>(elementCount);
        const size_t totalSize = sizeof(VertexInputRate) + sizeof(uint32_t) * 2 + elementsSize;

        std::vector<char> serializedData(totalSize);
        memset(serializedData.data(), 0, totalSize);

        memcpy(serializedData.data(), &_inputRate, sizeof(VertexInputRate));
        size_t pos = sizeof(VertexInputRate);

        memcpy(serializedData.data() + pos, &_binding, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        memcpy(serializedData.data() + pos, &elementCountU32, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        for (size_t i = 0; i < elementCount; ++i)
        {
            std::vector<char> serializedElement = _elements[i].serialize();
            PLATYPUS_ASSERT(serializedElement.size() == serializedElementSize);
            memcpy(serializedData.data() + pos, serializedElement.data(), serializedElementSize);
            pos += serializedElementSize;
            PLATYPUS_ASSERT(pos <= serializedData.size());
        }
        return serializedData;
    }

    VertexBufferLayout VertexBufferLayout::deserialize(
        const std::vector<char>& data,
        size_t offset
    )
    {
        const size_t baseSize = sizeof(VertexInputRate) + sizeof(uint32_t) * 2;
        PLATYPUS_ASSERT((offset + baseSize) < data.size());

        VertexInputRate inputRate;
        uint32_t binding = 0;
        uint32_t elementCount = 0;

        memcpy(&inputRate, data.data() + offset, sizeof(VertexInputRate));
        size_t pos = offset + sizeof(VertexInputRate);

        memcpy(&binding, data.data() + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        memcpy(&elementCount, data.data() + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        Debug::log(
            "___TEST___attempting to deserialize " + std::to_string(elementCount) + " elements "
            "for VertexBufferLayout"
        );
        std::vector<VertexBufferElement> elements;
        for (uint32_t i = 0; i < elementCount; ++i)
        {
            elements.push_back(VertexBufferElement::deserialize(data, pos));
            pos += VertexBufferElement::get_serialized_size();
        }

        // NOTE: Don't remember what overrideStride was used for...
        //  -> it had something to do with using only parts of vertex buffers instead of the
        //  whole buffer? For example in shadow pass, etc?
        return {
            elements,
            inputRate,
            binding,
            -1 // overrideStride
        };
    }

    size_t VertexBufferLayout::getSerializedSize() const
    {
        return sizeof(VertexInputRate) +
            sizeof(uint32_t) + // binding
            sizeof(uint32_t) + // elementCount
            VertexBufferElement::get_serialized_size() * _elements.size(); // elements
    }

    VertexBufferLayout VertexBufferLayout::get_common_static_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3, VertexAttributeType::POSITION },
                { 1, ShaderDataType::Float3, VertexAttributeType::NORMAL },
                { 2, ShaderDataType::Float2, VertexAttributeType::TEX_COORD }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_static_tangent_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3, VertexAttributeType::POSITION },
                { 1, ShaderDataType::Float3, VertexAttributeType::NORMAL },
                { 2, ShaderDataType::Float2, VertexAttributeType::TEX_COORD },
                { 3, ShaderDataType::Float4, VertexAttributeType::TANGENT }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_skinned_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3, VertexAttributeType::POSITION },
                { 1, ShaderDataType::Float4, VertexAttributeType::WEIGHT },
                { 2, ShaderDataType::Float4, VertexAttributeType::JOINT },
                { 3, ShaderDataType::Float3, VertexAttributeType::NORMAL },
                { 4, ShaderDataType::Float2, VertexAttributeType::TEX_COORD }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_skinned_tangent_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3, VertexAttributeType::POSITION },
                { 1, ShaderDataType::Float4, VertexAttributeType::WEIGHT },
                { 2, ShaderDataType::Float4, VertexAttributeType::JOINT },
                { 3, ShaderDataType::Float3, VertexAttributeType::NORMAL },
                { 4, ShaderDataType::Float2, VertexAttributeType::TEX_COORD },
                { 5, ShaderDataType::Float4, VertexAttributeType::TANGENT }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_skinned_shadow_layout(int32_t overrideStride)
    {
        return {
            {
                { 0, ShaderDataType::Float3, VertexAttributeType::POSITION },
                { 1, ShaderDataType::Float4, VertexAttributeType::WEIGHT },
                { 2, ShaderDataType::Float4, VertexAttributeType::JOINT }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0,
            overrideStride
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_terrain_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3, VertexAttributeType::POSITION },
                { 1, ShaderDataType::Float3, VertexAttributeType::NORMAL }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_terrain_tangent_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3, VertexAttributeType::POSITION },
                { 1, ShaderDataType::Float3, VertexAttributeType::NORMAL },
                { 2, ShaderDataType::Float3, VertexAttributeType::TEX_COORD } // NOTE: gltf meshes' tangent is vec4 since that's how we get it from the file, but the terrain is generated in a way that tangent is vec3... don't know why I'm doing this...
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }


    size_t get_shader_datatype_size(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Int:  return sizeof(int);
            case ShaderDataType::Int2: return sizeof(int) * 2;
            case ShaderDataType::Int3: return sizeof(int) * 3;
            case ShaderDataType::Int4: return sizeof(int) * 4;

            case ShaderDataType::Float:  return sizeof(float);
            case ShaderDataType::Float2: return sizeof(float) * 2;
            case ShaderDataType::Float3: return sizeof(float) * 3;
            case ShaderDataType::Float4: return sizeof(float) * 4;

            case ShaderDataType::Mat4: return sizeof(float) * 16;
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

    std::string shader_datatype_to_string(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Int:  return "Int";
            case ShaderDataType::Int2: return "Int2";
            case ShaderDataType::Int3: return "Int3";
            case ShaderDataType::Int4: return "Int4";

            case ShaderDataType::Float:  return "Float";
            case ShaderDataType::Float2: return "Float2";
            case ShaderDataType::Float3: return "Float3";
            case ShaderDataType::Float4: return "Float4";

            case ShaderDataType::Mat3: return "Mat3";
            case ShaderDataType::Mat4: return "Mat4";

            case ShaderDataType::Sampler2D: return "Sampler2D";

            default: return "Invalid type";
        }
    }

    size_t get_dynamic_uniform_buffer_element_size(size_t requestSize)
    {
        size_t alignRequirement = Device::get_min_uniform_buffer_offset_align();
        #ifdef PLATYPUS_DEBUG
        if (alignRequirement == 0)
        {
            Debug::log(
                "@get_dynamic_uniform_buffer_element_size "
                "Minimum uniform buffer offset alignment was 0! "
                "Make sure you have created the Device before quering this.",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return 0;
        }
        #endif
        size_t diff = (std::max(requestSize - 1, (size_t)1)) / alignRequirement;
        return  alignRequirement * (diff + 1);
    }

    std::string uniform_block_layout_to_string(UniformBlockLayout layout)
    {
        switch (layout)
        {
            case std140: return "std140";
            case std430: return "std430";
            default: return "<Invalid layout>";
        }
    }

    void Buffer::updateHost(void* pData, size_t dataSize, size_t offset)
    {
        if (!_pData)
        {
            Debug::log(
                "@Buffer::updateHost "
                "No buffer allocated host side!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        if (!validateUpdate(pData, dataSize, offset))
        {
            Debug::log(
                "@Buffer::updateHost "
                "Failed to update buffer!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        memcpy((void*)((PE_byte*)_pData + offset), pData, dataSize);
        _hostSideUpdated = true;
    }

    void Buffer::updateDeviceAndHost(void* pData, size_t dataSize, size_t offset)
    {
        updateHost(pData, dataSize, offset);
        updateDevice(pData, dataSize, offset);
    }

    bool Buffer::validateUpdate(void* pData, size_t dataSize, size_t offset)
    {
        if ((dataSize) > getTotalSize())
        {
            Debug::log(
                "@Buffer::validateUpdate "
                "provided data size(" + std::to_string(dataSize) + ") too big! "
                "Buffer size is " + std::to_string(getTotalSize()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        if (offset > getTotalSize())
        {
            Debug::log(
                "@Buffer::validateUpdate "
                "Offset " + std::to_string(offset) + " out of bounds "
                "for buffer with size " + std::to_string(getTotalSize()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }


}
