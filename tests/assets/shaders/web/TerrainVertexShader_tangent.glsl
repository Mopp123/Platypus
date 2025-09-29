precision mediump float;

attribute vec3 position;
attribute vec3 normal;
attribute vec3 tangent;

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
    vec2 meshProperties;
    // meshProperties.x = tileSize
    // meshProperties.y = verticesPerRow
};
uniform InstanceData instanceData;

varying vec3 var_normal;
varying vec2 var_texCoord;
varying vec3 var_fragPos; // in tangent space
varying vec3 var_toCamera; // in tangent space
varying vec3 var_lightDir; // in tangent space
varying vec4 var_lightColor;
varying vec4 var_ambientLightColor;

varying float var_tileSize;
varying float var_verticesPerRow;

varying mat3 var_toTangentSpace; // uses locations 9-11
varying vec4 var_tangent;

mat3 transpose(mat3 matrix)
{
    mat3 result;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result[i][j] = matrix[j][i];
        }
    }
    return result;
}

void main()
{
    mat4 toCameraSpace = sceneData.viewMatrix * instanceData.transformationMatrix;
    vec4 transformedPos = instanceData.transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformedPos;

    float tileSize = instanceData.meshProperties.x;
    float verticesPerRow = instanceData.meshProperties.y;
    float tilesPerRow = verticesPerRow - 1.0;
    var_texCoord = vec2(position.x / tileSize / tilesPerRow, position.z / tileSize / tilesPerRow);

    var_fragPos = transformedPos.xyz;

    vec3 transformedNormal = normalize(toCameraSpace * vec4(normal, 0.0)).xyz;
    vec3 transformedTangent = normalize((toCameraSpace * vec4(tangent.xyz, 0.0)).xyz);
    // T = normalize(T - dot(T, N) * N);
    transformedTangent = normalize(transformedTangent - dot(transformedTangent, transformedNormal) * transformedNormal);

    vec3 biTangent = normalize(cross(transformedNormal, transformedTangent));
    var_toTangentSpace = transpose(mat3(transformedTangent, biTangent, transformedNormal));

    vec3 toCam = sceneData.cameraPosition.xyz - transformedPos.xyz;
    var_toCamera = normalize(var_toTangentSpace * (sceneData.viewMatrix * vec4(toCam, 0.0)).xyz);

    var_lightDir = var_toTangentSpace * (sceneData.viewMatrix * vec4(sceneData.lightDirection.xyz, 0.0)).xyz;
    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;
    var_normal = (toCameraSpace * vec4(normal, 0.0)).xyz;

    var_tangent = vec4(biTangent, 1.0);

    var_tileSize = instanceData.meshProperties.x;
    var_verticesPerRow = instanceData.meshProperties.y;
}
