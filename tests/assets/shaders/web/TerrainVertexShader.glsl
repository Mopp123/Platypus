#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

struct SceneData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
    vec4 ambientLightColor;
};
uniform SceneData sceneData;


struct InstanceData
{
    mat4 transformationMatrix;
};
uniform InstanceData instanceData;

out vec3 var_normal;
out vec2 var_texCoord;
out vec3 var_fragPos;
out vec3 var_cameraPos;
out vec3 var_lightDir;
out vec4 var_lightColor;
out vec4 var_ambientLightColor;

void main()
{
    vec4 translatedPos = instanceData.transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * translatedPos;
    vec4 rotatedNormal = instanceData.transformationMatrix * vec4(normal, 0.0);
    var_normal = rotatedNormal.xyz;

    //float tileSize = instanceData.meshProperties.x;
    //float verticesPerRow = instanceData.meshProperties.y;
    //float tilesPerRow = verticesPerRow - 1.0;
    //var_texCoord = vec2(position.x / tileSize / tilesPerRow, position.z / tileSize / tilesPerRow);
    var_texCoord = texCoord;

    var_fragPos = translatedPos.xyz;
    var_cameraPos = sceneData.cameraPosition.xyz;

    var_lightDir = sceneData.lightDirection.xyz;
    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;
}
