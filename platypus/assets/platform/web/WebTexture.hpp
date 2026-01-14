#pragma once

#include "platypus/assets/Image.hpp"
#include <GL/glew.h>
#include <cstdint>


namespace platypus
{
    GLenum to_gl_format(ImageFormat format);

    struct TextureImpl
    {
        uint32_t id;
    };
}
