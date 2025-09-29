#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(set = 0, binding = 0) uniform SceneData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
    vec4 ambientLightColor;
} sceneData;

layout(set = 1, binding = 0) uniform InstanceData
{
    mat4 transformationMatrix;
    vec2 meshProperties;
    // meshProperties.x = tileSize
    // meshProperties.y = verticesPerRow
} instanceData;

layout(location = 0) out vec3 var_normal;
layout(location = 1) out vec2 var_texCoord;
layout(location = 2) out vec3 var_fragPos;
layout(location = 3) out vec3 var_cameraPos;
layout(location = 4) out vec3 var_lightDir;
layout(location = 5) out vec4 var_lightColor;
layout(location = 6) out vec4 var_ambientLightColor;

layout(location = 7) out float var_tileSize;
layout(location = 8) out float var_verticesPerRow;

void main()
{
    vec4 translatedPos = instanceData.transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * translatedPos;
    vec4 rotatedNormal = instanceData.transformationMatrix * vec4(normal, 0.0);
    var_normal = rotatedNormal.xyz;

    const float tileSize = instanceData.meshProperties.x;
    const float verticesPerRow = instanceData.meshProperties.y;
    const float tilesPerRow = verticesPerRow - 1;
    var_texCoord = vec2(position.x / tileSize / tilesPerRow, position.z / tileSize / tilesPerRow);

    var_fragPos = translatedPos.xyz;
    var_cameraPos = sceneData.cameraPosition.xyz;

    var_lightDir = sceneData.lightDirection.xyz;
    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;

    var_tileSize = instanceData.meshProperties.x;
    var_verticesPerRow = instanceData.meshProperties.y;
}
