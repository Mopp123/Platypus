#version 300 es
precision mediump float;

in vec2 var_texCoord;
in vec4 var_color;

uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 textureColor = texture(textureSampler, var_texCoord);
    float intensity = textureColor.a;

    vec4 totalColor = vec4(var_color.rgb, intensity);
    outColor = totalColor;
}
