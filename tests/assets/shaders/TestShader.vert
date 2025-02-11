#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

layout(push_constant) uniform Constants
{
    mat4 projectionMatrix;
} constants;

layout(set = 0, binding = 0) uniform Test
{
    mat4 transformationMatrix;
} test;

layout(location = 0) out vec2 var_texCoord;

void main() {
    vec4 translatedPos = test.transformationMatrix * vec4(position, 0.0, 1.0);
    gl_Position = constants.projectionMatrix * translatedPos;
    var_texCoord = texCoord;
}
