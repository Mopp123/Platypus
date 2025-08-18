precision mediump float;

varying vec3 var_normal;
varying vec2 var_texCoord;
varying vec3 var_fragPos;
varying vec3 var_cameraPos;
varying vec3 var_lightDir;
varying vec4 var_lightColor;

const vec4 ambientLight = vec4(0.1, 0.1, 0.1, 1);

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
uniform sampler2D diffuseTextureSampler;
uniform sampler2D specularTextureSampler;
struct MaterialData
{
    vec4 data;
    // x = specular strength
    // y = shininess
    // z = is shadeless
    // w = dunno...
};
uniform MaterialData materialData;

void main() {

    vec4 diffuseTextureColor = texture2D(diffuseTextureSampler, var_texCoord);
    vec4 specularTextureColor = texture2D(specularTextureSampler, var_texCoord);

    float specularStrength = materialData.data.x;
    float shininess = materialData.data.y;
    float isShadeless = materialData.data.z;

    vec3 unitLightDir = normalize(var_lightDir.xyz);
    vec3 toLight = -unitLightDir;
    vec3 unitNormal = normalize(var_normal);
    vec3 toCamera = normalize(var_cameraPos - var_fragPos);
    vec4 lightColor = vec4(var_lightColor.rgb, 1.0);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * lightColor;

    //vec3 reflectedLight = normalize(reflect(unitLightDir, unitNormal));
    vec3 halfWay = normalize(toLight + toCamera);

    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);
    vec4 specularColor = lightColor * specularFactor * specularStrength * specularTextureColor;

    vec4 finalColor = (ambientLight + lightDiffuseColor + specularColor) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }
    gl_FragColor = finalColor;
}
