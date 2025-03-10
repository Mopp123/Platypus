#include "Buffers.h"


// The reason for this file:
//  -> Needed to have some common funcs defined which applies to all implementations

namespace platypus
{
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

    size_t get_dynamic_uniform_buffer_element_size(size_t requestSize)
    {
        return requestSize;
    }
}
