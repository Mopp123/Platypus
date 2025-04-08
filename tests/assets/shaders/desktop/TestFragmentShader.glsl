#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;

layout(set = 0, binding = 0) uniform DirectionalLight
{
    vec4 direction;
    vec4 color;
} directionalLight;

const vec4 ambientLight = vec4(0.1, 0.1, 0.1, 1);

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 toLight = normalize(directionalLight.direction.xyz * -1.0);
    vec3 unitNormal = normalize(var_normal);
    float dirLightAmount = max(dot(toLight, unitNormal), 0.0);
    vec4 totalLightColor = ambientLight + dirLightAmount * vec4(directionalLight.color.rgb, 1.0);
    vec4 textureColor = texture(textureSampler, var_texCoord);

    vec4 finalColor = textureColor * totalLightColor;

    if (textureColor.a < 0.5)
    {
        discard;
    }

    fragColor = finalColor;
}
