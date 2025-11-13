#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;
layout(location = 2) in vec3 var_fragPos; // in tangent space
layout(location = 3) in vec3 var_toCamera; // in tangent space
layout(location = 4) in vec3 var_lightDir; // in tangent space
layout(location = 5) in vec4 var_lightColor;
layout(location = 6) in vec4 var_ambientLightColor;
layout(location = 7) in mat3 var_toTangentSpace; // uses locations 7-9
layout(location = 10) in vec4 var_tangent;

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 1, binding = 0) uniform sampler2D diffuseTextureSampler;
layout(set = 1, binding = 1) uniform sampler2D specularTextureSampler;
layout(set = 1, binding = 2) uniform sampler2D normalTextureSampler;
layout(set = 1, binding = 3) uniform MaterialData
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

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 finalTexCoord = var_texCoord * materialData.textureProperties.zw;
    finalTexCoord = finalTexCoord + materialData.textureProperties.xy;

    vec4 diffuseTextureColor = texture(diffuseTextureSampler, finalTexCoord);
    vec4 specularTextureColor = texture(specularTextureSampler, finalTexCoord);
    vec4 normalTextureColor = texture(normalTextureSampler, finalTexCoord);

    // Make it between -1 and 1
    vec3 normalMapNormal = normalTextureColor.rgb * 2.0 - 1.0;
    vec3 unitNormal = normalize(normalMapNormal);

    float specularStrength = materialData.lightingProperties.x;
    float shininess = materialData.lightingProperties.y;
    float isShadeless = materialData.lightingProperties.z;

    vec3 toLight = normalize(-var_lightDir);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * var_lightColor;

    //vec3 reflectedLight = normalize(reflect(unitLightDir, unitNormal));
    vec3 halfWay = normalize(toLight + var_toCamera);
    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);
    vec4 specularColor = var_lightColor * specularFactor * specularStrength * specularTextureColor;

    vec4 finalColor = (var_ambientLightColor + lightDiffuseColor + specularColor) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }
    outColor = finalColor;
}
