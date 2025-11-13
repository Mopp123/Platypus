#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in mat4 transformationMatrix;

struct PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform PushConstants pushConstants;

void main()
{
    gl_Position = pushConstants.projectionMatrix * pushConstants.viewMatrix * transformationMatrix * vec4(position, 1.0);
}
