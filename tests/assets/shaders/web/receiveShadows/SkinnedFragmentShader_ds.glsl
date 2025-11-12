#version 300 es
precision mediump float;

in vec3 var_normal;
in vec2 var_texCoord;
in vec3 var_fragPos;
in vec3 var_cameraPos;
in vec3 var_lightDir;
in vec4 var_lightColor;
in vec4 var_ambientLightColor;

in vec4 var_fragPosLightSpace;
in vec4 var_shadowProperties;

uniform sampler2D diffuseTextureSampler;
uniform sampler2D specularTextureSampler;
uniform sampler2D shadowmapTexture;
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


float calcShadow(float bias, int pcfCount)
{
    float shadow = 0.0;
    int shadowmapWidth = int(var_shadowProperties.x);
    int texelsCount_width = (2 * pcfCount + 1);
    int texelCount =  texelsCount_width * texelsCount_width;
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
            shadow += shadowmapCoord.z - bias > d ? 1.0 : 0.0;
        }
    }
    shadow /= float(texelCount);

    // that weird far plane shadow
    if (shadowmapCoord.z > 1.0)
        return 0.0;
    return shadow;
}

void main()
{
    vec2 finalTexCoord = var_texCoord * materialData.textureProperties.zw;
    finalTexCoord = finalTexCoord + materialData.textureProperties.xy;

    vec4 diffuseTextureColor = texture(diffuseTextureSampler, finalTexCoord);
    vec4 specularTextureColor = texture(specularTextureSampler, finalTexCoord);

    float specularStrength = materialData.lightingProperties.x;
    float shininess = materialData.lightingProperties.y;
    float isShadeless = materialData.lightingProperties.z;

    vec3 unitLightDir = normalize(var_lightDir.xyz);
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

    int shadowPCFSampleRadius = int(var_shadowProperties.y);
    float shadowStrength = var_shadowProperties.z;
    float bias = 0.0; //max(min((1.0 - dot(unitNormal, toLight)), maxBias), minBias);
    float shadow = min(calcShadow(bias, shadowPCFSampleRadius), shadowStrength);

    vec4 finalColor = (var_ambientLightColor + (1.0 - shadow) * lightDiffuseColor + specularColor) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }
    outColor = finalColor;
}
