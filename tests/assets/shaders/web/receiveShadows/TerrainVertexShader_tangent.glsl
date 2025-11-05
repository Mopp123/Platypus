#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 tangent;

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
    vec4 lightDirection;
    vec4 lightColor;
    vec4 ambientLightColor;
} sceneData;

layout(std140) uniform InstanceData
{
    mat4 transformationMatrix;
} instanceData;

out vec3 var_normal;
out vec2 var_texCoord;
out vec3 var_fragPos; // in tangent space
out vec3 var_toCamera; // in tangent space
out vec3 var_lightDir; // in tangent space
out vec4 var_lightColor;
out vec4 var_ambientLightColor;

out mat3 var_toTangentSpace; // uses locations 7-9
out vec4 var_tangent;

out vec3 var_shadowCoord;

void main()
{
    mat4 toCameraSpace = sceneData.viewMatrix * instanceData.transformationMatrix;
    vec4 transformedPos = instanceData.transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformedPos;

    //const float tileSize = instanceData.meshProperties.x;
    //const float verticesPerRow = instanceData.meshProperties.y;
    //const float tilesPerRow = verticesPerRow - 1.0;
    //var_texCoord = vec2(position.x / tileSize / tilesPerRow, position.z / tileSize / tilesPerRow);
    var_texCoord = texCoord;

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

    // NOTE: Not sure is this correct AND not sure should perspective division be done rather in fragment shader?!?!
	vec4 shadowCoord = shadowMatrices.projectionMatrix * shadowMatrices.viewMatrix * transformedPos;
    var_shadowCoord = shadowCoord.xyz / shadowCoord.w;
	var_shadowCoord = 0.5 + 0.5 * var_shadowCoord;
}
