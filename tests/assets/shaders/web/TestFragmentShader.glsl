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

uniform sampler2D textureSampler;

void main() {
    vec3 toLight = normalize(directionalLight.direction.xyz * -1.0);
    vec3 unitNormal = normalize(var_normal);
    float dirLightAmount = max(dot(toLight, unitNormal), 0.0);
    vec4 totalLightColor = ambientLight + dirLightAmount * vec4(directionalLight.color.rgb, 1.0);
    vec4 textureColor = texture2D(textureSampler, var_texCoord);

    vec4 finalColor = textureColor * totalLightColor;

    if (textureColor.a < 0.5)
    {
        discard;
    }

    gl_FragColor = finalColor;
}
