#pragma once

#include <string>
#include "platypus/graphics/Context.h"


namespace platypus
{
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


    class Pipeline;

    struct ShaderImpl;
    class Shader
    {
    private:
        friend class Pipeline;
        ShaderImpl* _pImpl = nullptr;
        ShaderStageFlagBits _stage = ShaderStageFlagBits::SHADER_STAGE_NONE;

    public:
        // Give shader's file name only EXCLUDING path and extension.
        // All shaders has to be located int assets/shaders/{PLATFORM} and this resolves the path and
        // extension depending on build target
        Shader(const std::string& filename, ShaderStageFlagBits stage);
        ~Shader();

        inline ShaderStageFlagBits getStage() const { return _stage; }
    };
}
