#version 450


//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 1, binding = 0) uniform sampler2D diffuseTextureSampler;
layout(set = 1, binding = 1) uniform sampler2D specularTextureSampler;
layout(set = 1, binding = 2) uniform MaterialData
{
    // x = specular strength
    // y = shininess
    // z = is shadeless
    // w = unused
    vec4 lightingProperties;

    // x,y = texture offset
    // z,w = texture scale
    vec4 textureProperties;
} materialData;


layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(1, 0, 0, 1);
}
