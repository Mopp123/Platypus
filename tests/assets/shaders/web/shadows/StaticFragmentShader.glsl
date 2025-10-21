precision mediump float;

uniform sampler2D diffuseTextureSampler;
uniform sampler2D specularTextureSampler;
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

void main()
{
    gl_FragColor = vec4(1, 0, 0, 1);
}
