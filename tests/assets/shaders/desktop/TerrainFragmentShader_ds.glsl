#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;
layout(location = 2) in vec3 var_fragPos;
layout(location = 3) in vec3 var_cameraPos;
layout(location = 4) in vec3 var_lightDir;
layout(location = 5) in vec4 var_lightColor;
layout(location = 6) in vec4 var_ambientLightColor;

layout(location = 7) in float var_tileSize;
layout(location = 8) in float var_verticesPerRow;

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 2, binding = 0) uniform sampler2D blendmapTexture;
layout(set = 2, binding = 1) uniform sampler2D diffuseTextureChannel0;
layout(set = 2, binding = 2) uniform sampler2D diffuseTextureChannel1;
layout(set = 2, binding = 3) uniform sampler2D diffuseTextureChannel2;
layout(set = 2, binding = 4) uniform sampler2D diffuseTextureChannel3;
layout(set = 2, binding = 5) uniform sampler2D diffuseTextureChannel4;

layout(set = 2, binding = 6) uniform sampler2D specularTextureChannel0;
layout(set = 2, binding = 7) uniform sampler2D specularTextureChannel1;
layout(set = 2, binding = 8) uniform sampler2D specularTextureChannel2;
layout(set = 2, binding = 9) uniform sampler2D specularTextureChannel3;
layout(set = 2, binding = 10) uniform sampler2D specularTextureChannel4;


// NOTE: Not sure if need to pass that kind of material stuff here just yet
layout(set = 2, binding = 11) uniform MaterialData
{
    vec4 data;
    // x = specular strength
    // y = shininess
    // z = is shadeless
    // w = dunno...
} materialData;


layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 tiledCoord = var_texCoord * (var_verticesPerRow - 1.0);
    vec4 blendmapColor = texture(blendmapTexture, var_texCoord);
    float transparency = 1.0 - blendmapColor.a;
    float blackAmount = max(1.0 - blendmapColor.r - blendmapColor.g - blendmapColor.b - transparency, 0.0);

    vec4 diffuseChannel0Color = texture(diffuseTextureChannel0, tiledCoord) * blackAmount;
    vec4 diffuseChannel1Color = texture(diffuseTextureChannel1, tiledCoord) * blendmapColor.r;
    vec4 diffuseChannel2Color = texture(diffuseTextureChannel2, tiledCoord) * blendmapColor.g;
    vec4 diffuseChannel3Color = texture(diffuseTextureChannel3, tiledCoord) * blendmapColor.b;
    vec4 diffuseChannel4Color = texture(diffuseTextureChannel4, tiledCoord) * transparency;

    vec4 specularChannel0Color = texture(specularTextureChannel0, tiledCoord) * blackAmount;
    vec4 specularChannel1Color = texture(specularTextureChannel1, tiledCoord) * blendmapColor.r;
    vec4 specularChannel2Color = texture(specularTextureChannel2, tiledCoord) * blendmapColor.g;
    vec4 specularChannel3Color = texture(specularTextureChannel3, tiledCoord) * blendmapColor.b;
    vec4 specularChannel4Color = texture(specularTextureChannel4, tiledCoord) * transparency;

    vec4 totalDiffuseColor = diffuseChannel0Color + diffuseChannel1Color + diffuseChannel2Color + diffuseChannel3Color + diffuseChannel4Color;
    vec4 totalSpecularColor = specularChannel0Color + specularChannel1Color + specularChannel2Color + specularChannel3Color + specularChannel4Color;

    //float specularStrength = materialData.data.x;
    //float shininess = materialData.data.y;
    float specularStrength = 0.75;
    float shininess = 32.0;
    float isShadeless = materialData.data.z;

    vec3 unitLightDir = normalize(var_lightDir.xyz);
    vec3 toLight = -unitLightDir;
    vec3 unitNormal = normalize(var_normal);
    vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);

    vec3 halfWay = normalize(toLight + toCamera);
    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);

    vec4 finalAmbientColor = var_ambientLightColor * totalDiffuseColor;
    vec4 finalDiffuseColor = lightColor * diffuseFactor * totalDiffuseColor;
    vec4 finalSpecularColor = lightColor * (specularFactor * specularStrength) * totalSpecularColor;

    fragColor = finalAmbientColor + finalDiffuseColor + finalSpecularColor;
}
