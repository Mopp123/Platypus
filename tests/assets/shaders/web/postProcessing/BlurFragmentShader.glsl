#version 300 es
precision mediump float;

in vec2[11] var_blurTexCoord;

uniform sampler2D textureSampler;
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 totalColor = vec4(0.0);
    totalColor += texture(textureSampler, var_blurTexCoord[0]) * 0.0093;
    totalColor += texture(textureSampler, var_blurTexCoord[1]) * 0.028002;
    totalColor += texture(textureSampler, var_blurTexCoord[2]) * 0.065984;
    totalColor += texture(textureSampler, var_blurTexCoord[3]) * 0.121703;
    totalColor += texture(textureSampler, var_blurTexCoord[4]) * 0.175713;

    totalColor += texture(textureSampler, var_blurTexCoord[5]) * 0.198596;

    totalColor += texture(textureSampler, var_blurTexCoord[6]) * 0.175713;
    totalColor += texture(textureSampler, var_blurTexCoord[7]) * 0.121703;
    totalColor += texture(textureSampler, var_blurTexCoord[8]) * 0.065984;
    totalColor += texture(textureSampler, var_blurTexCoord[9]) * 0.028002;
    totalColor += texture(textureSampler, var_blurTexCoord[10]) * 0.0093;

    outColor = totalColor;
    outColor.a = 1.0;
}
