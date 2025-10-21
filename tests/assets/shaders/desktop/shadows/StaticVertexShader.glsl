#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in mat4 transformationMatrix;

layout(set = 0, binding = 0) uniform SceneData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
    vec4 ambientLightColor;
} sceneData;

void main()
{
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformationMatrix * vec4(position, 1.0);
}
