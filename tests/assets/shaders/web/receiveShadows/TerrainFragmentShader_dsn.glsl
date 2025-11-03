#version 300 es
precision mediump float;

in vec3 var_normal;
in vec2 var_texCoord;
in vec3 var_fragPos; // in tangent space
in vec3 var_toCamera; // in tangent space
in vec3 var_lightDir; // in tangent space
in vec4 var_lightColor;
in vec4 var_ambientLightColor;

in mat3 var_toTangentSpace; // uses locations 7-9
in vec4 var_tangent;

in vec3 var_shadowCoord;

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

uniform sampler2D normalTextureChannel0;
uniform sampler2D normalTextureChannel1;
uniform sampler2D normalTextureChannel2;
uniform sampler2D normalTextureChannel3;
uniform sampler2D normalTextureChannel4;

uniform sampler2D shadowmapTexture;

struct MaterialData
{
    // x = specular strength
    // y = shininess
    // z = is shadeless
    // w = unused
    vec4 lightingProperties;

    // x,y = texture offset
    // z,w = texture scale
    vec4 textureProperties;
};
uniform MaterialData materialData;

layout(location = 0) out vec4 outColor;

void main()
{
    // NOTE: Not sure should offset be added to original coord or tiled coord...
    //  -> would be nice to be able to affect both separately!
    //vec2 tiledCoord = var_texCoord * (var_verticesPerRow - 1.0);
    vec2 tiledCoord = var_texCoord * materialData.textureProperties.zw;
    tiledCoord = tiledCoord + materialData.textureProperties.xy;

    vec4 blendmapColor = texture(blendmapTexture, var_texCoord);
    float transparency = 1.0 - blendmapColor.a;
    float blackAmount = max(1.0 - blendmapColor.r - blendmapColor.g - blendmapColor.b - transparency, 0.0);

    vec4 diffuseChannel0Color = texture(diffuseTextureChannel0, tiledCoord) * blackAmount;
    vec4 diffuseChannel1Color = texture(diffuseTextureChannel1, tiledCoord) * blendmapColor.r;
    vec4 diffuseChannel2Color = texture(diffuseTextureChannel2, tiledCoord) * blendmapColor.g;
    vec4 diffuseChannel3Color = texture(diffuseTextureChannel3, tiledCoord) * blendmapColor.b;
    vec4 diffuseChannel4Color = texture(diffuseTextureChannel4, tiledCoord) * transparency;

    vec4 specularChannel0Color = texture(specularTextureChannel0, tiledCoord) * blackAmount;
    vec4 specularChannel1Color = texture(specularTextureChannel1, tiledCoord) * blendmapColor.r;
    vec4 specularChannel2Color = texture(specularTextureChannel2, tiledCoord) * blendmapColor.g;
    vec4 specularChannel3Color = texture(specularTextureChannel3, tiledCoord) * blendmapColor.b;
    vec4 specularChannel4Color = texture(specularTextureChannel4, tiledCoord) * transparency;

    vec4 normalChannel0Color = texture(normalTextureChannel0, tiledCoord) * blackAmount;
    vec4 normalChannel1Color = texture(normalTextureChannel1, tiledCoord) * blendmapColor.r;
    vec4 normalChannel2Color = texture(normalTextureChannel2, tiledCoord) * blendmapColor.g;
    vec4 normalChannel3Color = texture(normalTextureChannel3, tiledCoord) * blendmapColor.b;
    vec4 normalChannel4Color = texture(normalTextureChannel4, tiledCoord) * transparency;

    // TODO: Better naming for these
    vec4 totalDiffuseColor = diffuseChannel0Color + diffuseChannel1Color + diffuseChannel2Color + diffuseChannel3Color + diffuseChannel4Color;
    vec4 totalSpecularColor = specularChannel0Color + specularChannel1Color + specularChannel2Color + specularChannel3Color + specularChannel4Color;
    vec4 totalNormalColor = normalChannel0Color + normalChannel1Color + normalChannel2Color + normalChannel3Color + normalChannel4Color;

    float specularStrength = materialData.lightingProperties.x;
    float shininess = materialData.lightingProperties.y;
    float isShadeless = materialData.lightingProperties.z;

    // Make it between -1 and 1
    vec3 normalMapNormal = totalNormalColor.rgb * 2.0 - 1.0;
    vec3 unitNormal = normalize(normalMapNormal);

    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    vec3 toLight = normalize(-var_lightDir);
    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);

    //vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    //vec3 halfWay = normalize(toLight + toCamera);
    vec3 halfWay = normalize(toLight + var_toCamera);
    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);

    vec4 finalAmbientColor = var_ambientLightColor * totalDiffuseColor;
    vec4 finalDiffuseColor = lightColor * diffuseFactor * totalDiffuseColor;
    vec4 finalSpecularColor = lightColor * (specularFactor * specularStrength) * totalSpecularColor;

    // Test shadow...
    float shadowMapVal = texture(shadowmapTexture, var_shadowCoord.xy).r;
    float shadow = var_shadowCoord.z > shadowMapVal ? 1.0 : 0.0;

    outColor = finalAmbientColor + (1.0 - shadow) * (finalDiffuseColor + finalSpecularColor);
}
