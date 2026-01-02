#version 300 es

precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(std140) uniform SceneData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 cameraPosition;

    vec4 ambientLightColor;
    vec4 lightDirection;
    vec4 lightColor;
    // x = shadowmap width, y = pcf sample radius, z = shadow strength, w = undetermined atm
    vec4 shadowProperties;

    float time;
} sceneData;

layout(std140) uniform InstanceData
{
    mat4 transformationMatrix;
} instanceData;

out vec3 var_normal;
out vec2 var_texCoord;
out vec3 var_fragPos;
out vec3 var_cameraPos;
out vec3 var_lightDir;
out vec4 var_lightColor;
out vec4 var_ambientLightColor;
out float var_time;
out vec4 var_clipPos;

void main() {
    vec4 translatedPos = instanceData.transformationMatrix * vec4(position, 1.0);
    var_clipPos = sceneData.projectionMatrix * sceneData.viewMatrix * translatedPos;
    gl_Position = var_clipPos;
    vec4 rotatedNormal = instanceData.transformationMatrix * vec4(normal, 0.0);
    var_normal = rotatedNormal.xyz;
    var_texCoord = texCoord;
    var_fragPos = translatedPos.xyz;
    var_cameraPos = sceneData.cameraPosition.xyz;

    var_lightDir = sceneData.lightDirection.xyz;
    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;

    var_time = sceneData.time;
}
