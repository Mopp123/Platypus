#include "Buffers.h"
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
        return _location == other._location && _type == other._type;
    }

    bool VertexBufferElement::operator!=(const VertexBufferElement& other) const
    {
        return _location != other._location || _type != other._type;
    }


    bool VertexBufferLayout::operator==(const VertexBufferLayout& other) const
    {
        return _elements == other._elements && _inputRate == other._inputRate && _stride == other._stride;
    }

    bool VertexBufferLayout::operator!=(const VertexBufferLayout& other) const
    {
        return _elements != other._elements || _inputRate != other._inputRate || _stride != other._stride;
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


    VertexBufferLayout VertexBufferLayout::get_common_static_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3 }, // position
                { 1, ShaderDataType::Float3 }, // normal
                { 2, ShaderDataType::Float2 }  // tex coord
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_static_tangent_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3 }, // position
                { 1, ShaderDataType::Float3 }, // normal
                { 2, ShaderDataType::Float2 }, // tex coord
                { 3, ShaderDataType::Float4 }  // tangent
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_skinned_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3 }, // position
                { 1, ShaderDataType::Float4 }, // weights
                { 2, ShaderDataType::Float4 }, // jointIDs
                { 3, ShaderDataType::Float3 }, // normal
                { 4, ShaderDataType::Float2 }  // tex coord
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_skinned_tangent_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3 }, // position
                { 1, ShaderDataType::Float4 }, // weights
                { 2, ShaderDataType::Float4 }, // jointIDs
                { 3, ShaderDataType::Float3 }, // normal
                { 4, ShaderDataType::Float2 }, // tex coord
                { 5, ShaderDataType::Float4 }  // tangent
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_skinned_shadow_layout(int32_t overrideStride)
    {
        return {
            {
                { 0, ShaderDataType::Float3 }, // position
                { 1, ShaderDataType::Float4 }, // weights
                { 2, ShaderDataType::Float4 } // jointIDs
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
                { 0, ShaderDataType::Float3 },
                { 1, ShaderDataType::Float3 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }

    VertexBufferLayout VertexBufferLayout::get_common_terrain_tangent_layout()
    {
        return {
            {
                { 0, ShaderDataType::Float3 },
                { 1, ShaderDataType::Float3 },
                { 2, ShaderDataType::Float3 } // NOTE: gltf meshes' tangent is vec4 since that's how we get it from the file, but the terrain is generated in a way that tangent is vec3... don't know why I'm doing this...
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
    }
}
