#include "Buffers.h"
#include "platypus/core/Debug.h"
#include "platypus/Common.h"
#include <cstring>


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

            case ShaderDataType::Mat4: return "Mat4";
            default: return "Invalid type";
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
