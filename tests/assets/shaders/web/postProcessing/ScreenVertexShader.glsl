#version 300 es
precision mediump float;

vec2 positions[6] = vec2[](
    vec2(-1.0, 1.0),
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),

    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0)
);

struct PushConstants
{
    float bloomIntensity;
};
uniform PushConstants pushConstants;

out vec2 var_texCoord;
out float var_bloomIntensity;

void main()
{
    vec2 vertexPos = positions[gl_VertexID];
    gl_Position = vec4(vertexPos, 0.0, 1.0);

    var_texCoord = vertexPos * 0.5 + 0.5;

    var_bloomIntensity = pushConstants.bloomIntensity;
}
