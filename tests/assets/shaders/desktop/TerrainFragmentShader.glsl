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
layout(set = 2, binding = 1) uniform sampler2D channel0Texture;
layout(set = 2, binding = 2) uniform sampler2D channel1Texture;
layout(set = 2, binding = 3) uniform sampler2D channel2Texture;
layout(set = 2, binding = 4) uniform sampler2D channel3Texture;
layout(set = 2, binding = 5) uniform sampler2D channel4Texture;

// NOTE: Not sure if need to pass that kind of material stuff here just yet
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

void main()
{
    vec2 tiledCoord = var_texCoord * (var_verticesPerRow - 1.0);
    vec4 blendmapColor = texture(blendmapTexture, var_texCoord);
    float blackAmount = max(1.0 - blendmapColor.r - blendmapColor.g - blendmapColor.b - blendmapColor.a, 0.0);

    vec4 blackChannelColor =    texture(channel0Texture, tiledCoord) * blackAmount;
    vec4 redChannelColor =      texture(channel1Texture, tiledCoord) * blendmapColor.r;

    vec4 greenChannelColor =    texture(channel2Texture, tiledCoord) * blendmapColor.g;
    vec4 blueChannelColor =     texture(channel3Texture, tiledCoord) * blendmapColor.b;
    vec4 alphaChannelColor =    texture(channel4Texture, tiledCoord) * blendmapColor.a;

    vec4 totalTextureColor = blackChannelColor + redChannelColor + greenChannelColor + blueChannelColor + alphaChannelColor;


    vec3 unitLightDir = normalize(var_lightDir.xyz);
    vec3 toLight = -unitLightDir;
    vec3 unitNormal = normalize(var_normal);
    vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * lightColor;

    //vec3 reflectedLight = normalize(reflect(unitLightDir, unitNormal));
    vec3 halfWay = normalize(toLight + toCamera);

    vec4 finalColor = (var_ambientLightColor + lightDiffuseColor) * totalTextureColor;

    fragColor = finalColor;
}
