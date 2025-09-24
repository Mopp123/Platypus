precision mediump float;

varying vec3 var_normal;
varying vec2 var_texCoord;
varying vec3 var_fragPos;
varying vec3 var_cameraPos;
varying vec3 var_lightDir;
varying vec4 var_lightColor;
varying vec4 var_ambientLightColor;

varying float var_tileSize;
varying float var_verticesPerRow;

uniform sampler2D blendmapTexture;
uniform sampler2D channel0Texture;
uniform sampler2D channel1Texture;
uniform sampler2D channel2Texture;
uniform sampler2D channel3Texture;
uniform sampler2D channel4Texture;


void main()
{
    vec2 tiledCoord = var_texCoord * (var_verticesPerRow - 1.0);
    vec4 blendmapColor = texture2D(blendmapTexture, var_texCoord);
    float blackAmount = max(1.0 - blendmapColor.r - blendmapColor.g - blendmapColor.b - blendmapColor.a, 0.0);

    vec4 blackChannelColor =    texture2D(channel0Texture, tiledCoord) * blackAmount;
    vec4 redChannelColor =      texture2D(channel1Texture, tiledCoord) * blendmapColor.r;

    vec4 greenChannelColor =    texture2D(channel2Texture, tiledCoord) * blendmapColor.g;
    vec4 blueChannelColor =     texture2D(channel3Texture, tiledCoord) * blendmapColor.b;
    vec4 alphaChannelColor =    texture2D(channel4Texture, tiledCoord) * blendmapColor.a;

    vec4 totalTextureColor = blackChannelColor + redChannelColor + greenChannelColor + blueChannelColor + alphaChannelColor;


    vec3 unitLightDir = normalize(var_lightDir.xyz);
    vec3 toLight = -unitLightDir;
    vec3 unitNormal = normalize(var_normal);
    vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * lightColor;

    //vec3 reflectedLight = normalize(reflect(unitLightDir, unitNormal));
    vec3 halfWay = normalize(toLight + toCamera);

    vec4 finalColor = (var_ambientLightColor + lightDiffuseColor) * totalTextureColor;

    gl_FragColor = finalColor;
}
