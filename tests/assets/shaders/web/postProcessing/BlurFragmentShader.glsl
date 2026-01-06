#version 300 es
precision mediump float;

#define BLUR_RADIUS 5
#define BLUR_SIZE (BLUR_RADIUS * 2 + 1)

in vec2[BLUR_SIZE] var_blurTexCoord;

uniform sampler2D textureSampler;
layout(location = 0) out vec4 outColor;


const float[5] g2 = float[](
    0.15338835280702454,
    0.22146110682534667,

    0.2503010807352574,

    0.22146110682534667,
    0.15338835280702454
);

const float[11] g5 = float[](
    0.0093,
    0.028002,
    0.065984,
    0.121703,
    0.175713,

    0.198596,

    0.175713,
    0.121703,
    0.065984,
    0.028002,
    0.0093
);

void main()
{
    vec4 totalColor = vec4(0.0);
    for (int i = 0; i < BLUR_SIZE; ++i)
    {
        totalColor += texture(textureSampler, var_blurTexCoord[i]) * g5[i];
    }

    outColor = totalColor;
    outColor.a = 1.0;
}
