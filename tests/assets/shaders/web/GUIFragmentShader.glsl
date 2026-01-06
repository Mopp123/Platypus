#version 300 es
precision mediump float;

in vec2 var_texCoord;
in vec4 var_color;

uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 textureColor = texture(textureSampler, var_texCoord) * var_color;

    float gamma = 2.2;
    vec4 applyGamma = vec4(gamma, gamma, gamma, 1.0);
    outColor = pow(textureColor, 1.0 / applyGamma);
}
