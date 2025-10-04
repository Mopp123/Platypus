#pragma once

#include "platypus/assets/Image.h"
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
