#pragma once

#include <string>
#include "platypus/graphics/Context.h"


namespace platypus
{
    struct ShaderImpl;

    enum ShaderStageFlagBits
    {
        SHADER_STAGE_NONE = 0,
        SHADER_STAGE_VERTEX_BIT = 0x1,
        SHADER_STAGE_FRAGMENT_BIT = 0x2
    };

    // Atm used for determining how to handle gl shader
    // attrib and uniform locations if required to handle
    // at all..
    enum class ShaderVersion
    {
        GLSL3,
        ESSL1
    };


    class Shader
    {
    private:
        ShaderImpl* _pImpl = nullptr;
        ShaderStageFlagBits _stage = ShaderStageFlagBits::SHADER_STAGE_NONE;

    public:
        Shader(const std::string& filepath, ShaderStageFlagBits stage);
        ~Shader();

        inline ShaderStageFlagBits getStage() const { return _stage; }
    };
}
