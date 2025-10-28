precision mediump float;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;

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

varying vec3 var_normal;
varying vec2 var_texCoord;
varying vec3 var_fragPos;
varying vec3 var_cameraPos;
varying vec3 var_lightDir;
varying vec4 var_lightColor;
varying vec4 var_ambientLightColor;

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
