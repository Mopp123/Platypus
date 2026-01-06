#version 450

vec2 positions[6] = vec2[](
    vec2(-1.0, 1.0),
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),

    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(-1.0, 1.0)
);

layout(location = 0) out vec2 var_texCoord;

void main()
{
    vec2 vertexPos = positions[gl_VertexIndex];
    gl_Position = vec4(vertexPos, 0.0, 1.0);

    var_texCoord = vertexPos * 0.5 + 0.5;
    var_texCoord.y = 1.0 - var_texCoord.y;
}
