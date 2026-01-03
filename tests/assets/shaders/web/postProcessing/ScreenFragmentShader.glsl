#version 300 es
precision mediump float;

in vec2 var_texCoord;

uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(textureSampler, var_texCoord);
}
