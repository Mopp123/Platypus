precision mediump float;

varying vec3 var_normal;
varying vec2 var_texCoord;

const vec4 ambientLight = vec4(0.1, 0.1, 0.1, 1);

struct DirectionalLight
{
    vec4 direction;
    vec4 color;
};
uniform DirectionalLight directionalLight;

uniform sampler2D diffuseTextureSampler;
uniform sampler2D specularTextureSampler;

void main() {

    vec4 diffuseTextureColor = texture2D(diffuseTextureSampler, var_texCoord);
    vec4 specularTextureColor = texture2D(specularTextureSampler, var_texCoord);

    vec3 toLight = normalize(directionalLight.direction.xyz * -1.0);
    vec3 unitNormal = normalize(var_normal);
    float diffuseFactor = max(dot(toLight, unitNormal), 0.0);
    vec4 lightDiffuseColor = diffuseFactor * vec4(directionalLight.color.rgb, 1.0);

    vec4 finalColor = (ambientLight + lightDiffuseColor) * diffuseTextureColor + (specularTextureColor * vec4(0.0, 0.0, 0.0, 0.0));

    if (diffuseTextureColor.a < 0.5)
    {
        discard;
    }

    gl_FragColor = finalColor;
}
