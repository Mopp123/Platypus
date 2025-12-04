#include "Shader.h"

namespace platypus
{
    std::string shader_stage_to_string(uint32_t shaderStage)
    {
        switch(shaderStage)
        {
            case ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT: return "Vertex Shader";
            case ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT: return "Fragment Shader";
            default: return "<Invalid shader stage>";
        }
    }

    std::string shader_version_to_string(ShaderVersion version)
    {
        switch (version)
        {
            case ShaderVersion::VULKAN_GLSL_450: return "VULKAN_GLSL_450";
            case ShaderVersion::OPENGLES_GLSL_300: return "OPENGLES_GLSL_300";
            default: return "<Invalid version>";
        }
    }

    // TODO: Make it impossible to attempt getting skinned shader name for terrain Material
    std::string get_default_complete_shader_filename(
        const std::string nameBegin,
        uint32_t shaderStage,
        bool hasSpecularMap,
        bool hasNormalMap,
        bool skinned,
        bool shadow
    )
    {
        // Example shader names:
        // vertex shader: "StaticVertexShader", "StaticVertexShader_tangent", "SkinnedVertexShader"
        // fragment shader: "StaticFragmentShader_d", "StaticFragmentShader_ds", "SkinnedFragmentShader_dsn"
        std::string shaderName = "";
        if (shadow)
            shaderName += "shadows/";

        shaderName += nameBegin;

        // Using same vertex shader for diffuse and diffuse+specular
        if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
        {
            shaderName += hasNormalMap ? "VertexShader" : "VertexShader_tangent";
        }
        else if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
        {
            // adding d here since all materials needs to have at least one diffuse texture
            shaderName += "FragmentShader_d";
            if (hasSpecularMap)
                shaderName += "s";
            if (hasNormalMap)
                shaderName += "n";
        }

        return shaderName;
    }
}
