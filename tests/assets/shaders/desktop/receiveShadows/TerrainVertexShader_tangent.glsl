#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 tangent;

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

layout(set = 1, binding = 0) uniform InstanceData
{
    mat4 transformationMatrix;
} instanceData;

layout(location = 0) out vec3 var_normal;
layout(location = 1) out vec2 var_texCoord;
layout(location = 2) out vec3 var_fragPos; // in tangent space
layout(location = 3) out vec3 var_toCamera; // in tangent space
layout(location = 4) out vec3 var_lightDir; // in tangent space
layout(location = 5) out vec4 var_lightColor;
layout(location = 6) out vec4 var_ambientLightColor;

layout(location = 7) out mat3 var_toTangentSpace; // uses locations 7-9
layout(location = 10) out vec4 var_tangent;

layout(location = 11) out vec3 var_shadowCoord;
layout(location = 12) out vec4 var_shadowProperties;

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
    shadowCoord.y *= -1.0;
    var_shadowCoord = shadowCoord.xyz / shadowCoord.w;
	var_shadowCoord = 0.5 + 0.5 * var_shadowCoord;

    var_shadowProperties = sceneData.shadowProperties;
}
