#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;
layout(location = 2) in vec3 var_fragPos;
layout(location = 3) in vec3 var_cameraPos;
layout(location = 4) in vec3 var_lightDir;
layout(location = 5) in vec4 var_lightColor;
layout(location = 6) in vec4 var_ambientLightColor;

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 1, binding = 0) uniform sampler2D diffuseTextureSampler;
layout(set = 1, binding = 1) uniform sampler2D specularTextureSampler;
layout(set = 1, binding = 2) uniform MaterialData
{
    vec4 data;
    // x = specular strength
    // y = shininess
    // z = is shadeless
    // w = dunno...
} materialData;


layout(location = 0) out vec4 fragColor;

void main() {

    vec4 diffuseTextureColor = texture(diffuseTextureSampler, var_texCoord);
    vec4 specularTextureColor = texture(specularTextureSampler, var_texCoord);

    float specularStrength = materialData.data.x;
    float shininess = materialData.data.y;
    float isShadeless = materialData.data.z;

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

    vec4 finalColor = (var_ambientLightColor + lightDiffuseColor + specularColor) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }
    fragColor = finalColor;
}
