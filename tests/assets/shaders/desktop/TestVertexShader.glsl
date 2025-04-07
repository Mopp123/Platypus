#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 transformationMatrix;

layout(push_constant) uniform Constants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
} constants;

layout(location = 0) out vec3 var_normal;
layout(location = 1) out vec2 var_texCoord;

void main() {
    vec4 translatedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = constants.projectionMatrix * constants.viewMatrix * translatedPos;
    var_normal = normal;
    var_texCoord = texCoord;
}
