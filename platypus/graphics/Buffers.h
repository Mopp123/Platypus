#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>


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
        BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x4
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


    size_t get_shader_datatype_size(ShaderDataType type);
    uint32_t get_shader_datatype_component_count(ShaderDataType type);

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
            std::vector<VertexBufferElement> elements,
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
        void* _data = nullptr;
        size_t _dataElemSize = 0; // size of a single entry in data
        size_t _dataLength = 0; // number of elements in the data
        uint32_t _bufferUsageFlags = 0;
        BufferUpdateFrequency _updateFrequency;

    public:
        // *NOTE! "elementSize" single element's size in "data buffer"
        // *NOTE! "dataLength" number of "elements" in the "data buffer" (NOT total size)
        // *NOTE! "Data" gets just copied here! Ownership of the data doesn't get transferred here!
        // (This is to accomplish RAII(and resource lifetimes to be tied to objects' lifetimes) and copying to work correctly)
        Buffer(
            void* data,
            size_t elementSize,
            size_t dataLength,
            uint32_t usageFlags,
            BufferUpdateFrequency updateFrequency,
            bool saveDataHostSide = false
        );
        Buffer(const Buffer&) = delete;
        ~Buffer();

        inline const void* getData() const { return _data; }
        inline size_t getDataElemSize() const { return _dataElemSize; }
        inline size_t getDataLength() const { return _dataLength; }
        inline uint32_t getBufferUsage() const { return _bufferUsageFlags; }
        inline size_t getTotalSize() const { return _dataElemSize * _dataLength; }
        inline BufferUpdateFrequency getUpdateFrequency() const { return _updateFrequency; }
    };
}
