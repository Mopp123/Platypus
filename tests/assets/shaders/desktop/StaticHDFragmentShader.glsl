#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;
layout(location = 2) in vec3 var_fragPos; // in tangent space
layout(location = 3) in vec3 var_toCamera; // in tangent space
layout(location = 4) in vec3 var_lightDir; // in tangent space
layout(location = 5) in vec4 var_lightColor; // in tangent space
layout(location = 6) in mat3 var_toTangentSpace;
layout(location = 9) in vec4 var_tangent;

const vec4 ambientLight = vec4(0.1, 0.1, 0.1, 1);

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(set = 2, binding = 0) uniform sampler2D diffuseTextureSampler;
layout(set = 2, binding = 1) uniform sampler2D specularTextureSampler;
layout(set = 2, binding = 2) uniform sampler2D normalTextureSampler;
layout(set = 2, binding = 3) uniform MaterialData
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
    vec4 normalTextureColor = texture(normalTextureSampler, var_texCoord);

    // Make it between -1 and 1
    vec3 normalMapNormal = normalTextureColor.rgb * 2.0 - 1.0;

    float specularStrength = materialData.data.x;
    float shininess = materialData.data.y;
    float isShadeless = materialData.data.z;

    vec3 toLight = normalize(-var_lightDir);
    vec3 unitNormal = normalize(normalMapNormal);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * var_lightColor;

    //vec3 reflectedLight = normalize(reflect(unitLightDir, unitNormal));
    vec3 halfWay = normalize(toLight + var_toCamera);

    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);
    vec4 specularColor = var_lightColor * specularFactor * specularStrength * specularTextureColor;

    vec4 finalColor = (ambientLight + lightDiffuseColor + specularColor) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }
    fragColor = finalColor;
}
