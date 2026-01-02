#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;
layout(location = 2) in vec3 var_fragPos;
layout(location = 3) in vec3 var_cameraPos;
layout(location = 4) in vec3 var_lightDir;
layout(location = 5) in vec4 var_lightColor;
layout(location = 6) in vec4 var_ambientLightColor;
layout(location = 7) in float var_time;
layout(location = 8) in vec4 var_clipPos;

layout(set = 2, binding = 0) uniform sampler2D diffuseTextureSampler; // Maybe don't use this at all?
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

    const float waveMultiplier = 0.25;
    float distortionSpeed = var_time * 0.01;
    vec2 distortedCoord1 = (texture(distortionTextureSampler, vec2(finalTexCoord.x + distortionSpeed, finalTexCoord.y)).rg * 2.0 - 1.0) * waveMultiplier;
    vec2 distortedCoord2 = (texture(distortionTextureSampler, vec2(finalTexCoord.x, finalTexCoord.y + distortionSpeed)).rg * 2.0 - 1.0) * waveMultiplier;
    vec3 distortedColor = texture(diffuseTextureSampler, distortedCoord1 + distortedCoord2).rgb;


    // TESTING DEPTH EFFECT
    // *Not perfect, but 'll do for now
    float zNear = 0.1;
    float zFar = 100.0;

    vec4 tempClip = var_clipPos;
    tempClip.y *= -1.0;
    vec2 ndcCoord = (tempClip.xy / tempClip.w) * 0.5 + 0.5;

    float depth = texture(depthMap, ndcCoord).r;
    float distToBottom = 2.0 * zNear * zFar / (zFar + zNear - (2.0 * depth - 1.0) * (zFar - zNear));

    depth = gl_FragCoord.z;
    float distToSurface = 2.0 * zNear * zFar / (zFar + zNear - (2.0 * depth - 1.0) * (zFar - zNear));

    float waterDepth = distToBottom - distToSurface;

    float d = clamp(waterDepth / 2.0, 0.0, 1.0);

    vec4 shallowTint = vec4(0.6, 0.6, 1.0, 1.0);
    vec4 deepTint = vec4(0.2, 0.2, 0.8, 1.0);
    vec4 totalTint = mix(shallowTint, deepTint, d);

    //vec4 textureColor = vec4(distortedColor, 1.0);
    vec4 textureColor = mix(vec4(distortedColor, 1.0), totalTint, 0.25);

    vec4 finalDiffuseColor = lightDiffuseColor * textureColor;
    vec4 finalSpecularColor = lightSpecularColor * textureColor;
    vec4 finalColor = var_ambientLightColor + finalDiffuseColor + finalSpecularColor;
    float transparency = mix(clamp(waterDepth, 0.0, 1.0), clamp(1.0 - fresnelEffect, 0.0, 1.0), 0.3);
    finalColor.a = transparency;

    outColor = finalColor;
}
