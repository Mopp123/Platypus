#version 450

layout(location = 0) in vec3 position;

layout (push_constant) uniform PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
} pushConstants;

layout (set = 0, binding = 0) uniform InstanceData
{
    mat4 transformationMatrix;
} instanceData;

void main()
{
    gl_Position = pushConstants.projectionMatrix * pushConstants.viewMatrix * instanceData.transformationMatrix * vec4(position, 1.0);
}
