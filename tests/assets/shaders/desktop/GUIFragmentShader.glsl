#version 450

layout(location = 0) in vec2 var_texCoord;

layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 textureColor = texture(textureSampler, var_texCoord);
    fragColor = textureColor;
}
