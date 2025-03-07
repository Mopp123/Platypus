#pragma once

#include <string>

namespace platypus
{
    unsigned int to_gl_shader(ShaderStageFlagBits stage);

    // For single shader stage (vertex, fragment)
    // *Not to be confused with the whole "combined opengl shader program"
    struct ShaderImpl
    {
        // We want to store the "shader source string" to get uniform names and locations!
        // We'll clear that after we get those uniforms
        std::string source;
        uint32_t id = 0;
    };
}
