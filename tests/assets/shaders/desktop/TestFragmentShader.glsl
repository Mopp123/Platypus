#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;

layout(set = 0, binding = 0) uniform DirectionalLight
{
    vec4 direction;
    vec4 color;
} directionalLight;

const vec4 ambientLight = vec4(0.1, 0.1, 0.1, 1);

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 1, binding = 0) uniform sampler2D diffuseTextureSampler;
layout(set = 1, binding = 1) uniform sampler2D specularTextureSampler;
/*
layout(set = 1, binding = 2) uniform MaterialData
{
    vec4 data;
    // x = specular strength
    // y = shininess
    // z = is shadeless
    // w = dunno...
} materialData;
*/


layout(location = 0) out vec4 fragColor;

void main() {

    vec4 diffuseTextureColor = texture(diffuseTextureSampler, var_texCoord);
    vec4 specularTextureColor = texture(specularTextureSampler, var_texCoord);

    //float specularStrength = materialData.data.x;
    //float shininess = materialData.data.y;
    //float isShadeless = materialData.data.z;

    vec3 toLight = normalize(directionalLight.direction.xyz * -1.0);
    vec3 unitNormal = normalize(var_normal);
    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);

    vec4 lightDiffuseColor = diffuseFactor * vec4(directionalLight.color.rgb, 1.0);

    vec4 finalColor = (ambientLight + lightDiffuseColor) * diffuseTextureColor;
    finalColor = clamp(finalColor, 0, 1);

    if (diffuseTextureColor.a < 0.5)
    {
        discard;
    }

    fragColor = finalColor;
}
