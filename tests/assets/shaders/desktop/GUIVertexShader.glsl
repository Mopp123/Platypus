#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in mat4 transformationMatrix;

layout(push_constant) uniform Constants
{
    mat4 projectionMatrix;
} constants;

layout(location = 0) out vec2 var_texCoord;

void main() {
    gl_Position = constants.projectionMatrix * transformationMatrix * vec4(position, 1.0);
    var_texCoord = texCoord;
}
