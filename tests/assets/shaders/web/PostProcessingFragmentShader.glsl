#version 300 es
precision mediump float;


in vec2 var_texCoord;

uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    float gamma = 2.2;
    vec4 applyGamma = vec4(gamma, gamma, gamma, 1.0);
    outColor = pow(texture(textureSampler, var_texCoord), 1.0 / applyGamma);
}
