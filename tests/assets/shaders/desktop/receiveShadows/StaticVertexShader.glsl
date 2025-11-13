#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat4 transformationMatrix;

layout (push_constant) uniform PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
} shadowMatrices;

layout(set = 0, binding = 0) uniform SceneData
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

layout(location = 0) out vec3 var_normal;
layout(location = 1) out vec2 var_texCoord;
layout(location = 2) out vec3 var_fragPos;
layout(location = 3) out vec3 var_cameraPos;
layout(location = 4) out vec3 var_lightDir;
layout(location = 5) out vec4 var_lightColor;
layout(location = 6) out vec4 var_ambientLightColor;

layout(location = 7) out vec4 var_fragPosLightSpace;
layout(location = 8) out vec4 var_shadowProperties;

void main() {
    vec4 transformedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformedPos;
    vec4 rotatedNormal = transformationMatrix * vec4(normal, 0.0);
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
