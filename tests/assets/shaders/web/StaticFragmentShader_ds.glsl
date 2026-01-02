#version 300 es
precision mediump float;

in vec3 var_normal;
in vec2 var_texCoord;
in vec3 var_fragPos;
in vec3 var_cameraPos;
in vec3 var_lightDir;
in vec4 var_lightColor;
in vec4 var_ambientLightColor;

uniform sampler2D diffuseTextureSampler;
uniform sampler2D specularTextureSampler;

layout( std140 ) uniform MaterialData
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

    float specularStrength = materialData.lightingProperties.x;
    float shininess = materialData.lightingProperties.y;
    float isShadeless = materialData.lightingProperties.z;

    vec3 unitLightDir = normalize(var_lightDir);
    vec3 toLight = -unitLightDir;
    vec3 unitNormal = normalize(var_normal);
    vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * lightColor;

    //vec3 reflectedLight = normalize(reflect(unitLightDir, unitNormal));
    vec3 halfWay = normalize(toLight + toCamera);

    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);
    vec4 specularColor = lightColor * specularFactor * specularStrength * specularTextureColor;

    vec4 finalColor = (var_ambientLightColor + lightDiffuseColor + specularColor) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }

    outColor = finalColor;;
}
