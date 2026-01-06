#version 300 es
precision mediump float;

#define BLUR_RADIUS 5
#define BLUR_SIZE (BLUR_RADIUS * 2 + 1)

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
    float fboWidth;
};
uniform PushConstants pushConstants;

out vec2[BLUR_SIZE] var_blurTexCoord;

void main()
{
    vec2 vertexPos = positions[gl_VertexID];
    gl_Position = vec4(vertexPos, 0.0, 1.0);
    float pixelSize = 1.0 / pushConstants.fboWidth;

    vec2 texCoord = vertexPos * 0.5 + 0.5;

    for (int i = -BLUR_RADIUS; i <= BLUR_RADIUS; ++i)
    {
        var_blurTexCoord[i + BLUR_RADIUS] = texCoord + vec2(0.0, pixelSize * float(i));
    }
}
