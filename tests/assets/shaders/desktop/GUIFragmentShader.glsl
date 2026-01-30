#version 450

layout(location = 0) in vec2 var_texCoord;
layout(location = 1) in vec4 var_color;
layout(location = 2) in vec2 var_pos;
layout(location = 3) in vec2 var_scale;
layout(location = 4) in vec4 var_borderColor;
layout(location = 5) in float var_borderThickness;

layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 textureColor = texture(textureSampler, var_texCoord) * var_color;
    // *Was able to get rid of previously used if statement with followin
    vec2 inBounds = step(vec2(var_borderThickness), var_pos) * step(var_pos, var_scale - var_borderThickness);
    float mask = min(inBounds.x, inBounds.y);
    outColor = mix(var_borderColor, textureColor, mask);
}
