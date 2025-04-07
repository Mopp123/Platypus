#include "platypus/graphics/Buffers.h"
#include "WebBuffers.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Context.h"
#include "WebContext.h"
#include "platypus/Common.h"
#include <cstdlib>
#include <GL/glew.h>


namespace platypus
{
    size_t get_dynamic_uniform_buffer_element_size(size_t requestSize)
    {
        return requestSize;
    }


    static GLenum to_opengl_buffer_update_frequency(BufferUpdateFrequency f)
    {
        switch (f)
        {
            case BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC:	return GL_STATIC_DRAW;
            case BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC: return GL_DYNAMIC_DRAW;
            case BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STREAM: return GL_STREAM_DRAW;
            default:
                Debug::log(
                    "@to_opengl_buffer_update_frequency "
                    "Invalid buffer update frequency: " + std::to_string(f),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return GL_STATIC_DRAW;
        }
    }


    struct VertexBufferElementImpl
    {
    };

    VertexBufferElement::VertexBufferElement()
    {
    }

    VertexBufferElement::VertexBufferElement(uint32_t location, ShaderDataType dataType) :
        _location(location),
        _type(dataType)
    {
    }

    VertexBufferElement::VertexBufferElement(const VertexBufferElement& other) :
        _location(other._location),
        _type(other._type)
    {
    }

    VertexBufferElement::~VertexBufferElement()
    {
    }


    struct VertexBufferLayoutImpl
    {
    };

    VertexBufferLayout::VertexBufferLayout(
        std::vector<VertexBufferElement> elements,
        VertexInputRate inputRate,
        uint32_t binding
    ) :
        _elements(elements),
        _inputRate(inputRate)
    {
        for (const VertexBufferElement& element : elements)
            _stride += get_shader_datatype_size(element.getType());
    }

    VertexBufferLayout::VertexBufferLayout(const VertexBufferLayout& other) :
        _elements(other._elements),
        _inputRate(other._inputRate),
        _stride(other._stride)
    {
    }

    VertexBufferLayout::~VertexBufferLayout()
    {
    }

    Buffer::Buffer(
        const CommandPool& commandPool,
        void* pData,
        size_t elementSize,
        size_t dataLength,
        uint32_t usageFlags,
        BufferUpdateFrequency updateFrequency
    ) :
        _dataElemSize(elementSize),
        _dataLength(dataLength),
        _bufferUsageFlags(usageFlags),
        _updateFrequency(updateFrequency)
    {
        // Need to bind the common VAO
        GL_FUNC(glBindVertexArray(Context::get_instance()->getImpl()->vaoID));

        // in gl terms its the glBufferData's "usage"
        GLenum glBufferUpdateFrequency = to_opengl_buffer_update_frequency(updateFrequency);
        uint32_t id = 0;
        if ((usageFlags & BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT) == BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT)
        {
            GL_FUNC(glGenBuffers(1, &id));
            GL_FUNC(glBindBuffer(GL_ARRAY_BUFFER, id));
            GL_FUNC(glBufferData(GL_ARRAY_BUFFER, getTotalSize(), pData, glBufferUpdateFrequency));
            GL_FUNC(glBindBuffer(GL_ARRAY_BUFFER, 0));
        }
        else if ((usageFlags & BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT) == BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT)
        {
            GL_FUNC(glGenBuffers(1, &id));
            GL_FUNC(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
            GL_FUNC(glBufferData(GL_ELEMENT_ARRAY_BUFFER, getTotalSize(), pData, GL_STATIC_DRAW));
            GL_FUNC(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        }
        else if (usageFlags & BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            _pData = calloc(_dataLength, _dataElemSize);
            memcpy(_pData, pData, _dataLength * _dataElemSize);
        }

        _pImpl = new BufferImpl { id };
    }

    Buffer::~Buffer()
    {
        if (_pData)
            free(_pData);

        if (_pImpl)
        {
            GL_FUNC(glDeleteBuffers(1, &_pImpl->id));
            delete _pImpl;
        }
    }

    // NOTE: With both update funcs, don't remember the reasoning behind waiting
    // the actual buffer updating(glBufferData/glBufferSubData) until render commands...
    // TODO: Test and figure out if it would be better to do it here!
    void Buffer::update(void* pData, size_t dataSize)
    {
        if (dataSize != getTotalSize())
        {
            Debug::log(
                "@Buffer::update(1) "
                "provided data size(" + std::to_string(dataSize) + ") different "
                "from original(" + std::to_string(getTotalSize()) + ") "
                "Currently web implementation requires you to replace the original "
                "completely with this function!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        if ((_bufferUsageFlags & BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT) == BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            memcpy(_pData, pData, dataSize);
        }
        else
        {
            Debug::log("@Buffer::update(1)(non uniform buffer) NOT TESTED!", Debug::MessageType::PLATYPUS_WARNING);
            GL_FUNC(glBindBuffer(GL_ARRAY_BUFFER, _pImpl->id));
            GLenum glBufferUpdateFrequency = to_opengl_buffer_update_frequency(_updateFrequency);
            GL_FUNC(glBufferData(GL_ARRAY_BUFFER, getTotalSize(), pData, glBufferUpdateFrequency));
        }
    }

    void Buffer::update(void* pData, size_t dataSize, size_t offset)
    {
        if (dataSize + offset > getTotalSize())
        {
            Debug::log(
                "@Buffer::update(2) "
                "provided data size(" + std::to_string(dataSize) + ") too big using offset: " + std::to_string(offset) + " "
                "Buffer size is " + std::to_string(getTotalSize()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        if ((_bufferUsageFlags & BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT) == BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            memcpy(((PE_byte*)_pData) + offset, pData, dataSize);
        }
        else
        {
            GL_FUNC(glBindBuffer(GL_ARRAY_BUFFER, _pImpl->id));
            glBufferSubData(
                GL_ARRAY_BUFFER,
                (GLintptr)offset,
                (GLsizeiptr) dataSize,
                pData
            );
        }
    }
}
