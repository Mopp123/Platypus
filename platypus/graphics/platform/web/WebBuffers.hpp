#pragma once

#include <GL/glew.h>
#include <set>
#include <string>


namespace platypus
{
    struct BufferImpl
    {
        uint32_t id;
        std::set<uint32_t> vaos;
    };

    GLenum to_opengl_buffer_type(uint32_t bufferUsageFlagBits);
    std::string opengl_buffer_type_to_string(GLenum type);
}
