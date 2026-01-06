#version 450

layout(location = 0) in vec2 var_texCoord;
layout(location = 1) in float var_bloomIntensity;

// NOTE: vertical blur texture has already horizontal blur in it!
layout(set = 0, binding = 0) uniform sampler2D verticalBlurTexture;
layout(set = 0, binding = 1) uniform sampler2D sceneTexture;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 verticalBlurColor = texture(verticalBlurTexture, var_texCoord);
    vec4 sceneTextureColor = texture(sceneTexture, var_texCoord);
    outColor = sceneTextureColor + verticalBlurColor * var_bloomIntensity;
}
