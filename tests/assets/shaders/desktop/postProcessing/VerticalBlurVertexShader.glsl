#version 450

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0),

    vec2(1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0)
);

vec2 uv[6] = vec2[](
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),

    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

layout(push_constant) uniform PushConstants
{
    float fboWidth;
} pushConstants;

const int blurRadius = 5;
// using radius of 5
layout(location = 0) out vec2[11] var_blurTexCoord;

void main()
{
    vec2 vertexPos = positions[gl_VertexIndex];
    gl_Position = vec4(vertexPos, 0.0, 1.0);
    float pixelSize = 1.0 / pushConstants.fboWidth;

    //var_texCoord = uv[gl_VertexIndex];
    vec2 texCoord = uv[gl_VertexIndex];

    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        var_blurTexCoord[i + blurRadius] = texCoord + vec2(0.0, pixelSize * float(i));
    }
}
