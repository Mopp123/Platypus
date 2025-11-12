#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;
layout(location = 2) in vec3 var_fragPos;
layout(location = 3) in vec3 var_cameraPos;
layout(location = 4) in vec3 var_lightDir;
layout(location = 5) in vec4 var_lightColor;
layout(location = 6) in vec4 var_ambientLightColor;

layout(location = 7) in vec4 var_shadowCoord;
layout(location = 8) in vec4 var_shadowProperties;

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 1, binding = 0) uniform sampler2D diffuseTextureSampler;
layout(set = 1, binding = 1) uniform sampler2D specularTextureSampler;
layout(set = 1, binding = 2) uniform sampler2D shadowmapTexture;
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

float calcShadow(float bias, int pcfCount, vec3 shadowCoord)
{
    float shadow = 0.0;
    int shadowmapWidth = int(var_shadowProperties.x);
    int texelsCount_width = (2 * pcfCount + 1);
    int texelCount =  texelsCount_width * texelsCount_width;
    vec2 texelSize = 1.0 / vec2(shadowmapWidth, shadowmapWidth);

    for (int x = -pcfCount; x <= pcfCount; x++)
    {
        for (int y = -pcfCount; y <= pcfCount; y++)
        {
            vec2 sampleCoord = shadowCoord.xy + vec2(x, y) * texelSize;
            if (sampleCoord.x > 1.0 || sampleCoord.x < 0.0 || sampleCoord.y > 1.0 || sampleCoord.y < 0.0)
                continue;

            float d = texture(shadowmapTexture, sampleCoord).r;
            shadow += var_shadowCoord.z - bias > d ? 1.0 : 0.0;
        }
    }
    shadow /= float(texelCount);

    // that weird far plane shadow
    if (var_shadowCoord.z > 1.0)
        return 0.0;
    return shadow;
}

float test(vec4 shadowCoord, vec2 off)
{
    float shadow = 1.0;
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
    {
        float dist = texture( shadowmapTexture, shadowCoord.st + off ).r;
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z )
        {
            shadow = 1;
        }
    }
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
    float bias = max(0.025 * (1.0 - dot(unitNormal, toLight)), 0.005);

    // WHY THE FUCK DOES THIS WORK!!?!?!?!?!?!?!
    vec4 t = var_shadowCoord;
    t.y *= -1.0;
    vec3 c = t.xyz / t.w;
    c = 0.5 + 0.5 * c;
    float shadow = min(calcShadow(bias, shadowPCFSampleRadius, c), shadowStrength);

    //float shadow = test(var_shadowCoord / var_shadowCoord.w, vec2(0, 0));

    vec4 finalColor = (var_ambientLightColor + (1.0 - shadow) * (lightDiffuseColor + specularColor)) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }
    outColor = finalColor;
}
