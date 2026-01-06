#version 300 es
precision mediump float;

in vec2 var_texCoord;
in float var_bloomIntensity;

// NOTE: vertical blur texture has already horizontal blur in it!
uniform sampler2D verticalBlurTexture;
uniform sampler2D sceneTexture;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 verticalBlurColor = texture(verticalBlurTexture, var_texCoord);
    vec4 sceneTextureColor = texture(sceneTexture, var_texCoord);
    vec4 totalColor = sceneTextureColor + verticalBlurColor * var_bloomIntensity;

    float gamma = 2.2;
    vec4 applyGamma = vec4(gamma, gamma, gamma, 1.0);
    outColor = pow(totalColor, 1.0 / applyGamma);
}
