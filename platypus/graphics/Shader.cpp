#include "Shader.h"

namespace platypus
{
    std::string shader_stage_to_string(uint32_t shaderStage)
    {
        switch(shaderStage)
        {
            case ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT: return "Vertex Shader";
            case ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT: return "Fragment Shader";
            default: return "Invalid Shader Stage";
        }
    }
}
