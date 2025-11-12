#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 weights;
layout(location = 2) in vec4 jointIDs;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec2 texCoord;

struct PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform PushConstants shadowMatrices;

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
} sceneData;

const int maxJoints = 50;
layout(std140) uniform JointData
{
    mat4 data[maxJoints];
} jointData;

out vec3 var_normal;
out vec2 var_texCoord;
out vec3 var_fragPos;
out vec3 var_cameraPos;
out vec3 var_lightDir;
out vec4 var_lightColor;
out vec4 var_ambientLightColor;

out vec4 var_fragPosLightSpace;
out vec4 var_shadowProperties;

void main() {
    float weightSum = weights[0] + weights[1] + weights[2] + weights[3];
    mat4 jointTransform = jointData.data[0];
    if (weightSum >= 1.0)
    {
        jointTransform =  jointData.data[int(jointIDs[0])] * weights[0];
        jointTransform += jointData.data[int(jointIDs[1])] * weights[1];
        jointTransform += jointData.data[int(jointIDs[2])] * weights[2];
        jointTransform += jointData.data[int(jointIDs[3])] * weights[3];
    }
    else
    {
        jointTransform = jointData.data[int(jointIDs[0])];
    }

    vec4 transformedPos = jointTransform * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformedPos;
    vec4 rotatedNormal = jointTransform * vec4(normal, 0.0);

    var_normal = rotatedNormal.xyz;
    var_texCoord = texCoord;
    var_fragPos = transformedPos.xyz;
    var_cameraPos = sceneData.cameraPosition.xyz;

    var_lightDir = sceneData.lightDirection.xyz;
    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;

    var_fragPosLightSpace = shadowMatrices.projectionMatrix * shadowMatrices.viewMatrix * transformedPos;
    var_shadowProperties = sceneData.shadowProperties;
}
