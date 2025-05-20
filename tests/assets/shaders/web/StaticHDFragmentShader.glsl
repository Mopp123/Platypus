precision mediump float;

varying vec2 var_texCoord;
varying vec3 var_toCamera; // in tangent space
varying vec3 var_lightDir; // in tangent space
varying vec4 var_lightColor;

const vec4 ambientLight = vec4(0.1, 0.1, 0.1, 1);

//layout(set = 1, binding = 0) uniform sampler2D textureSampler;
uniform sampler2D diffuseTextureSampler;
uniform sampler2D specularTextureSampler;
uniform sampler2D normalTextureSampler;
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
    vec4 normalTextureColor = texture2D(normalTextureSampler, var_texCoord);

    // Make it between -1 and 1
    vec3 normalMapNormal = normalTextureColor.rgb * 2.0 - 1.0;

    float specularStrength = materialData.data.x;
    float shininess = materialData.data.y;
    float isShadeless = materialData.data.z;

    vec3 toLight = normalize(-var_lightDir);
    vec3 unitNormal = normalize(normalMapNormal);

    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * var_lightColor;

    //vec3 reflectedLight = normalize(reflect(unitLightDir, unitNormal));
    vec3 halfWay = normalize(toLight + var_toCamera);

    float specularFactor = pow(max(dot(unitNormal, halfWay), 0.0), shininess);
    vec4 specularColor = var_lightColor * specularFactor * specularStrength * specularTextureColor;

    vec4 finalColor = (ambientLight + lightDiffuseColor + specularColor) * diffuseTextureColor;

    if (diffuseTextureColor.a < 0.1)
    {
        discard;
    }
    gl_FragColor = finalColor;
}
