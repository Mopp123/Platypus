#version 450

layout(location = 0) in vec3 var_normal;
layout(location = 1) in vec2 var_texCoord;

const vec4 ambientLight = vec4(0.1, 0.1, 0.1, 1);

layout(set = 1, binding = 0) uniform DirectionalLight
{
    vec4 direction;
    vec4 color;
} directionalLight;

layout(set = 2, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 toLight = normalize(directionalLight.direction.xyz * -1.0);
    vec3 unitNormal = normalize(var_normal);

    float dirLightAmount = dot(toLight, unitNormal);

    vec4 textureColor = texture(textureSampler, var_texCoord);

    fragColor = textureColor * (ambientLight + dirLightAmount * directionalLight.color);
}
