#version 300 es
precision mediump float;

in vec2 var_texCoord;
in vec3 var_toCamera; // in tangent space
in vec3 var_lightDir; // in tangent space
in vec4 var_lightColor;
in vec4 var_ambientLightColor;

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
uniform sampler2D diffuseTextureSampler;
uniform sampler2D specularTextureSampler;
uniform sampler2D normalTextureSampler;
layout(std140) uniform MaterialData
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

    float specularStrength = materialData.lightingProperties.x;
    float shininess = materialData.lightingProperties.y;
    float isShadeless = materialData.lightingProperties.z;

    vec3 toLight = normalize(-var_lightDir);
    vec3 unitNormal = normalize(normalMapNormal);

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
