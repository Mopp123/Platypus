#version 450

layout(location = 0) in vec2 var_texCoord;

layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 textureColor = texture(textureSampler, var_texCoord);
    float brightness = (textureColor.r * 0.2126) + (textureColor.g * 0.7152) + (textureColor.b * 0.0722);
    //float brightness = (textureColor.r + textureColor.g + textureColor.b) / 3.0;
    //outColor = vec4(brightness, brightness, brightness, 1.0);
    outColor = textureColor * brightness;
}
