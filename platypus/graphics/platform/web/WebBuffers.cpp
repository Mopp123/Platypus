#include "platypus/graphics/Buffers.h"
#include "WebBuffers.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Context.hpp"
#include "WebContext.hpp"
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

    VertexBufferElement& VertexBufferElement::operator=(VertexBufferElement&& other)
    {
        _location = other._location;
        _type = other._type;
        return *this;
    }

    VertexBufferElement& VertexBufferElement::operator=(const VertexBufferElement& other)
    {
        _location = other._location;
        _type = other._type;
        return *this;
    }

    VertexBufferElement::~VertexBufferElement()
    {
    }


    struct VertexBufferLayoutImpl
    {
    };

    VertexBufferLayout::VertexBufferLayout()
    {
    }

    VertexBufferLayout::VertexBufferLayout(
        const std::vector<VertexBufferElement>& elements,
        VertexInputRate inputRate,
        uint32_t binding,
        int32_t overrideStride
    ) :
        _elements(elements),
        _inputRate(inputRate)
    {
        if (overrideStride != -1)
        {
            _stride = overrideStride;
        }
        else
        {
            for (const VertexBufferElement& element : elements)
                _stride += get_shader_datatype_size(element.getType());
        }
    }

    VertexBufferLayout::VertexBufferLayout(const VertexBufferLayout& other) :
        _elements(other._elements),
        _inputRate(other._inputRate),
        _stride(other._stride)
    {
    }

    VertexBufferLayout& VertexBufferLayout::operator=(VertexBufferLayout&& other)
    {
        _elements.resize(other._elements.size());
        for (size_t i = 0; i < other._elements.size(); ++i)
        {
            _elements[i] = other._elements[i];
        }
        _inputRate = other._inputRate;
        _stride = other._stride;

        return *this;
    }

    VertexBufferLayout& VertexBufferLayout::operator=(const VertexBufferLayout& other)
    {
        _elements.resize(other._elements.size());
        for (size_t i = 0; i < other._elements.size(); ++i)
        {
            _elements[i] = other._elements[i];
        }
        _inputRate = other._inputRate;
        _stride = other._stride;

        return *this;
    }

    VertexBufferLayout::~VertexBufferLayout()
    {
    }


    Buffer::Buffer(
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
            uint32_t vboID = _pImpl->id;
            GL_FUNC(glDeleteBuffers(1, &vboID));
            if ((_bufferUsageFlags & BUFFER_USAGE_VERTEX_BUFFER_BIT) == BUFFER_USAGE_VERTEX_BUFFER_BIT)
            {
                // Remove from context impl's vaos
                // -> IF context impl's vao doesn't have any buffers associated with it anymore
                //  -> destroy the actual vao
                ContextImpl* pContextImpl = Context::get_impl();
                if (!pContextImpl)
                {
                    Debug::log("@Buffer::~Buffer Context impl was nullptr", Debug::MessageType::PLATYPUS_ERROR);
                    PLATYPUS_ASSERT(false);
                }
                for (uint32_t vaoID : _pImpl->vaos)
                {
                    pContextImpl->vaoDataMapping[vaoID].bufferIDs.erase(vboID);
                    if (vao_deletion_allowed(pContextImpl, vaoID))
                    {
                        GL_FUNC(glDeleteVertexArrays(1, &vaoID));
                        pContextImpl->vaoDataMapping.erase(vaoID);
                    }
                }
            }
            delete _pImpl;
        }
    }

    void Buffer::updateDevice(void* pData, size_t dataSize, size_t offset)
    {
        // Since we just fake uniform buffers here, the host side buffer used as uniform data should
        // already be updated at this point, so we don't need to do anything to those here...
        if ((_bufferUsageFlags & BUFFER_USAGE_UNIFORM_BUFFER_BIT) != BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
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

            GL_FUNC(glBindBuffer(GL_ARRAY_BUFFER, _pImpl->id));
            GLenum glBufferUpdateFrequency = to_opengl_buffer_update_frequency(_updateFrequency);
            if (getTotalSize() == dataSize)
            {
                GL_FUNC(glBufferData(GL_ARRAY_BUFFER, getTotalSize(), pData, glBufferUpdateFrequency));
            }
            else
            {
                GL_FUNC(glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)dataSize, pData));
            }
        }
    }
}
