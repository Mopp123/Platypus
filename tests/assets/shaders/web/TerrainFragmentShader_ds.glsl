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
uniform sampler2D diffuseTextureChannel0;
uniform sampler2D diffuseTextureChannel1;
uniform sampler2D diffuseTextureChannel2;
uniform sampler2D diffuseTextureChannel3;
uniform sampler2D diffuseTextureChannel4;

uniform sampler2D specularTextureChannel0;
uniform sampler2D specularTextureChannel1;
uniform sampler2D specularTextureChannel2;
uniform sampler2D specularTextureChannel3;
uniform sampler2D specularTextureChannel4;

struct MaterialData
{
    vec4 data;
    // x = specular strength
    // y = shininess
    // z = is shadeless
    // w = dunno...
};
uniform MaterialData materialData;

void main()
{
    vec2 tiledCoord = var_texCoord * (var_verticesPerRow - 1.0);
    vec4 blendmapColor = texture2D(blendmapTexture, var_texCoord);
    float transparency = 1.0 - blendmapColor.a;
    float blackAmount = max(1.0 - blendmapColor.r - blendmapColor.g - blendmapColor.b - transparency, 0.0);

    vec4 diffuseChannel0Color = texture2D(diffuseTextureChannel0, tiledCoord) * blackAmount;
    vec4 diffuseChannel1Color = texture2D(diffuseTextureChannel1, tiledCoord) * blendmapColor.r;
    vec4 diffuseChannel2Color = texture2D(diffuseTextureChannel2, tiledCoord) * blendmapColor.g;
    vec4 diffuseChannel3Color = texture2D(diffuseTextureChannel3, tiledCoord) * blendmapColor.b;
    vec4 diffuseChannel4Color = texture2D(diffuseTextureChannel4, tiledCoord) * transparency;

    vec4 specularChannel0Color = texture2D(specularTextureChannel0, tiledCoord) * blackAmount;
    vec4 specularChannel1Color = texture2D(specularTextureChannel1, tiledCoord) * blendmapColor.r;
    vec4 specularChannel2Color = texture2D(specularTextureChannel2, tiledCoord) * blendmapColor.g;
    vec4 specularChannel3Color = texture2D(specularTextureChannel3, tiledCoord) * blendmapColor.b;
    vec4 specularChannel4Color = texture2D(specularTextureChannel4, tiledCoord) * transparency;

    vec4 totalDiffuseColor = diffuseChannel0Color + diffuseChannel1Color + diffuseChannel2Color + diffuseChannel3Color + diffuseChannel4Color;
    vec4 totalSpecularColor = specularChannel0Color + specularChannel1Color + specularChannel2Color + specularChannel3Color + specularChannel4Color;

    //float specularStrength = materialData.data.x;
    //float shininess = materialData.data.y;
    float specularStrength = 0.75;
    float shininess = 32.0;
    float isShadeless = materialData.data.z;

    vec3 unitLightDir = normalize(var_lightDir.xyz);
    vec3 toLight = -unitLightDir;
    vec3 unitNormal = normalize(var_normal);
    vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);

    vec3 halfWay = normalize(toLight + toCamera);
    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);

    vec4 finalAmbientColor = var_ambientLightColor * totalDiffuseColor;
    vec4 finalDiffuseColor = lightColor * diffuseFactor * totalDiffuseColor;
    vec4 finalSpecularColor = lightColor * (specularFactor * specularStrength) * totalSpecularColor;

    gl_FragColor = finalAmbientColor + finalDiffuseColor + finalSpecularColor;
}
