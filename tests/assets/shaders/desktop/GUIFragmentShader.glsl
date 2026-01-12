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

    if (var_pos.x > var_borderThickness && var_pos.x < var_scale.x - var_borderThickness &&
        var_pos.y > var_borderThickness && var_pos.y < var_scale.y - var_borderThickness)
    {
        outColor = textureColor;
    }
    else
    {
        outColor = var_borderColor;
    }
}
