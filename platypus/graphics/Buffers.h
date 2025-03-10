#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include "CommandBuffer.h"


namespace platypus
{
    enum ShaderDataType
    {
        None = 0,

        Int,
        Int2,
        Int3,
        Int4,

        Float,
        Float2,
        Float3,
        Float4,

        // NOTE: Only used by opengl(?)
        Mat4
    };


    enum VertexInputRate
    {
        VERTEX_INPUT_RATE_VERTEX = 0,
        VERTEX_INPUT_RATE_INSTANCE = 1
    };


    enum BufferUsageFlagBits
    {
        BUFFER_USAGE_NONE = 0x0,
        BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x1,
        BUFFER_USAGE_INDEX_BUFFER_BIT = 0x2,
        BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x4,
        BUFFER_USAGE_TRANSFER_SRC_BIT = 0x8,
        BUFFER_USAGE_TRANSFER_DST_BIT = 0x10
    };


    enum BufferUpdateFrequency
    {
        BUFFER_UPDATE_FREQUENCY_STATIC = 0,
        BUFFER_UPDATE_FREQUENCY_DYNAMIC,
        BUFFER_UPDATE_FREQUENCY_STREAM
    };


    enum IndexType
    {
        INDEX_TYPE_NONE = 0,
        INDEX_TYPE_UINT16 = 1,
        INDEX_TYPE_UINT32 = 2
    };


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
    // On Vulkan side this returns required actual size for inputted requestSize
    // satisfying the minUniformBufferOffsetAlignment requirement.
    //  -> In other words, returns a multiple of the requirement
    // On OpenGL this returns the inputted requestSize
    size_t get_dynamic_uniform_buffer_element_size(size_t requestSize);

    class Pipeline;

    struct VertexBufferElementImpl;
    class VertexBufferElement
    {
    private:
        friend class VertexBufferLayout;
        friend class Pipeline;
        VertexBufferElementImpl* _pImpl = nullptr;
        uint32_t _location = 0;
        ShaderDataType _type = ShaderDataType::Float;

    public:
        VertexBufferElement();
        VertexBufferElement(uint32_t location, ShaderDataType dataType);
        VertexBufferElement(const VertexBufferElement&);
        ~VertexBufferElement();

        inline uint32_t getLocation() const { return _location; }
        inline ShaderDataType getType() const { return _type; }
    };


    struct VertexBufferLayoutImpl;
    class VertexBufferLayout
    {
    private:
        friend class Pipeline;
        VertexBufferLayoutImpl* _pImpl = nullptr;
        std::vector<VertexBufferElement> _elements;
        VertexInputRate _inputRate = VertexInputRate::VERTEX_INPUT_RATE_VERTEX;
        int32_t _stride = 0;

    public:
        // NOTE: Not sure if copying elems goes correctly here..
        VertexBufferLayout(
            std::vector<VertexBufferElement> elements, // Why not const ref?
            VertexInputRate inputRate,
            uint32_t binding
        );
        VertexBufferLayout(const VertexBufferLayout& other);
        ~VertexBufferLayout();

        inline const std::vector<VertexBufferElement>& getElements() const { return _elements; }
        inline VertexInputRate getInputRate() const { return _inputRate; }
        inline int32_t getStride() const { return _stride; }
    };


    struct BufferImpl;
    class Buffer
    {
    private:
        BufferImpl* _pImpl = nullptr;
        // NOTE: On OpenGL side, we need to save the _pData "on host side" to get "descriptor sets" working
        void* _pData = nullptr;
        size_t _dataElemSize = 0; // size of a single entry in data
        size_t _dataLength = 0; // number of elements in the data
        uint32_t _bufferUsageFlags = 0;
        BufferUpdateFrequency _updateFrequency;

    public:
        // NOTE:
        //  * "elementSize" single element's size in "data buffer"
        //  * "dataLength" number of "elements" in the "data buffer" (NOT total size)
        //  * "Data" gets just copied here! Ownership of the data doesn't get transferred here!
        //  * If usageFlags contains BUFFER_USAGE_TRANSFER_DST_BIT this will implicitly create, transfer
        //  and destroy staging buffer
        Buffer(
            const CommandPool& commandPool,
            void* pData,
            size_t elementSize,
            size_t dataLength,
            uint32_t usageFlags,
            BufferUpdateFrequency updateFrequency
        );
        Buffer(const Buffer&) = delete;
        ~Buffer();

        void update(void* pData, size_t dataSize);
        void update(void* pData, size_t dataSize, size_t offset);

        inline const void* getData() const { return _pData; }
        inline size_t getDataElemSize() const { return _dataElemSize; }
        inline size_t getDataLength() const { return _dataLength; }
        inline uint32_t getBufferUsage() const { return _bufferUsageFlags; }
        inline size_t getTotalSize() const { return _dataElemSize * _dataLength; }
        inline BufferUpdateFrequency getUpdateFrequency() const { return _updateFrequency; }

        inline BufferImpl* getImpl() const { return _pImpl; }
    };
}
