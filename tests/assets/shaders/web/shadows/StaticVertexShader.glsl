#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;

struct PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform PushConstants pushConstants;

layout (std140) uniform InstanceData
{
    mat4 transformationMatrix;
} instanceData;

void main()
{
    gl_Position = pushConstants.projectionMatrix * pushConstants.viewMatrix * instanceData.transformationMatrix * vec4(position, 1.0);
}
