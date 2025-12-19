#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;
layout(location = 2) in vec3 var_fragPos;
layout(location = 3) in vec3 var_cameraPos;
layout(location = 4) in vec3 var_lightDir;
layout(location = 5) in vec4 var_lightColor;
layout(location = 6) in vec4 var_ambientLightColor;
layout(location = 7) in float var_time;

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 2, binding = 0) uniform sampler2D diffuseTextureSampler;
layout(set = 2, binding = 1) uniform sampler2D distortionTextureSampler;
layout(set = 2, binding = 2) uniform sampler2D depthMap;
layout(set = 2, binding = 3) uniform MaterialData
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


    float specularStrength = materialData.lightingProperties.x;
    float shininess = materialData.lightingProperties.y;
    // TODO: if shadeless -> make actually shadeless!
    float isShadeless = materialData.lightingProperties.z;

    vec3 unitLightDir = normalize(var_lightDir.xyz);
    vec3 toLight = -unitLightDir;
    vec3 unitNormal = normalize(var_normal);
    vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * lightColor;

    vec3 halfWay = normalize(toLight + toCamera);
    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);
    vec4 lightSpecularColor = lightColor * specularFactor * specularStrength;

    float fresnelEffect = dot(toCamera, unitNormal);

    const float waveMultiplier = 0.5;
    float distortionSpeed = var_time * 0.01;
    vec2 distortedCoord1 = (texture(distortionTextureSampler, vec2(finalTexCoord.x + distortionSpeed, finalTexCoord.y)).rg * 2.0 - 1.0) * waveMultiplier;
    vec2 distortedCoord2 = (texture(distortionTextureSampler, vec2(finalTexCoord.x - distortionSpeed, finalTexCoord.y + distortionSpeed)).rg * 2.0 - 1.0) * waveMultiplier;
    vec3 distortedColor = texture(diffuseTextureSampler, distortedCoord1 + distortedCoord2).rgb;

    vec4 textureColor = vec4(distortedColor, 1.0);
    vec4 finalDiffuseColor = lightDiffuseColor * textureColor;
    vec4 finalSpecularColor = lightSpecularColor * textureColor;
    vec4 finalColor = var_ambientLightColor + finalDiffuseColor + finalSpecularColor;
    finalColor.a = 1.0 - fresnelEffect;

    outColor = finalColor;
}
