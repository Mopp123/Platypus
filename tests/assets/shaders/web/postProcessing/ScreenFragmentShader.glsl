#version 300 es
precision mediump float;

 in vec2 var_texCoord;

uniform sampler2D horizontalBlurTexture;
uniform sampler2D sceneTexture;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 horizontalBlurColor = texture(horizontalBlurTexture, var_texCoord);
    vec4 sceneTextureColor = texture(sceneTexture, var_texCoord);
    outColor = horizontalBlurColor;
}
