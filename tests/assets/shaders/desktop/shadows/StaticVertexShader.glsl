#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in mat4 transformationMatrix;

layout (push_constant) uniform PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
} pushConstants;

void main()
{
    gl_Position = pushConstants.projectionMatrix * pushConstants.viewMatrix * transformationMatrix * vec4(position, 1.0);
}
