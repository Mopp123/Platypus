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

    float dirLightAmount = dot(toLight, unitNormal);

    vec4 textureColor = texture2D(textureSampler, var_texCoord);

    gl_FragColor = textureColor * (ambientLight + dirLightAmount * directionalLight.color);
}
