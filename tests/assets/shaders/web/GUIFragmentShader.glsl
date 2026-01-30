#version 300 es
precision mediump float;

in vec2 var_texCoord;
in vec4 var_color;
in vec2 var_pos;
in vec2 var_scale;
in vec4 var_borderColor;
in float var_borderThickness;

uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 textureColor = texture(textureSampler, var_texCoord) * var_color;

    // *Was able to get rid of previously used if statement with followin
    vec2 inBounds = step(vec2(var_borderThickness), var_pos) * step(var_pos, var_scale - var_borderThickness);
    float mask = min(inBounds.x, inBounds.y);
    outColor = mix(var_borderColor, textureColor, mask);

    float gamma = 2.2;
    vec4 applyGamma = vec4(gamma, gamma, gamma, 1.0);
    outColor = pow(outColor, 1.0 / applyGamma);
}
