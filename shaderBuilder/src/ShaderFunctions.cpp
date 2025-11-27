#include "ShaderFunctions.hpp"
#include "ShaderBuilder.hpp"
#include <sstream>
#include <unordered_map>


namespace platypus
{
    namespace shaderBuilder
    {

        static std::unordered_map<ShaderVersion, std::unordered_map<std::string, std::string>> s_functions;

        static const std::string s_source_calcShadows_vulkan = R"(float calcShadow(float bias, int pcfCount)
{
    float shadow = 0.0;
    int shadowmapWidth = int(var_shadowProperties.x);
    int texelsWidth = (2 * pcfCount + 1);
    int texelCount =  texelsWidth * texelsWidth;
    vec2 texelSize = 1.0 / vec2(shadowmapWidth, shadowmapWidth);

    // WHY THE FUCK DOES THIS WORK!!?!?!?!?!?!?!
    // UPDATE: The thing with the Vulkan clip space compared to OpenGL...
    vec4 flippedFragPosLightSpace = var_fragPosLightSpace;
    flippedFragPosLightSpace.y *= -1.0;
    vec3 shadowmapCoord = flippedFragPosLightSpace.xyz / flippedFragPosLightSpace.w;
    shadowmapCoord = 0.5 + 0.5 * shadowmapCoord;

    for (int x = -pcfCount; x <= pcfCount; x++)
    {
        for (int y = -pcfCount; y <= pcfCount; y++)
        {
            vec2 sampleCoord = shadowmapCoord.xy + vec2(x, y) * texelSize;
            if (sampleCoord.x > 1.0 || sampleCoord.x < 0.0 || sampleCoord.y > 1.0 || sampleCoord.y < 0.0)
                continue;

            float d = texture(shadowmapTexture, sampleCoord).r;
            shadow += var_fragPosLightSpace.z > d + bias  ? 1.0 : 0.0;
        }
    }
    shadow /= float(texelCount);

    // that weird far plane shadow
    if (var_fragPosLightSpace.z > 1.0)
        return 0.0;
    return shadow;
})";

    static const std::string s_source_calcShadows_gl = R"(float calcShadow(float bias, int pcfCount)
{
    float shadow = 0.0;
    int shadowmapWidth = int(var_shadowProperties.x);
    int texelsWidth = (2 * pcfCount + 1);
    int texelCount =  texelsWidth * texelsWidth;
    vec2 texelSize = 1.0 / vec2(shadowmapWidth, shadowmapWidth);

    vec3 shadowmapCoord = var_fragPosLightSpace.xyz / var_fragPosLightSpace.w;
    shadowmapCoord = 0.5 + 0.5 * shadowmapCoord;

    for (int x = -pcfCount; x <= pcfCount; x++)
    {
        for (int y = -pcfCount; y <= pcfCount; y++)
        {
            vec2 sampleCoord = shadowmapCoord.xy + vec2(x, y) * texelSize;
            if (sampleCoord.x > 1.0 || sampleCoord.x < 0.0 || sampleCoord.y > 1.0 || sampleCoord.y < 0.0)
                continue;

            float d = texture(shadowmapTexture, sampleCoord).r;
            shadow += shadowmapCoord.z > d + bias  ? 1.0 : 0.0;
        }
    }
    shadow /= float(texelCount);

    // that weird far plane shadow
    if (shadowmapCoord.z > 1.0)
        return 0.0;
    return shadow;
})";

        std::vector<std::string> get_func_def(ShaderVersion shaderVersion, const std::string& funcName)
        {
            std::string useSource;
            // Atm calcShadow func is the only special case where it has separate definitions
            // for vulkan and gl
            if (funcName == ShaderStageBuilder::getFunctions().calcShadow)
            {
                if (shaderVersion == ShaderVersion::VULKAN_GLSL_450)
                    useSource = s_source_calcShadows_vulkan;
                else if (shaderVersion == ShaderVersion::OPENGLES_GLSL_300)
                    useSource = s_source_calcShadows_gl;
            }

            std::vector<std::string> outLines;

            std::istringstream s(useSource);
            std::string line;
            while(getline(s, line))
                outLines.push_back(line);

            return outLines;
        }
    }
}
