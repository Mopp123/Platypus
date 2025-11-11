#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 tangent;
layout(location = 4) in mat4 transformationMatrix;

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

out vec2 var_texCoord;
out vec3 var_toCamera; // in tangent space
out vec3 var_lightDir; // in tangent space
out vec4 var_lightColor;
out vec4 var_ambientLightColor;

// OLD BELOW -> seems solved for now..
// NOTE: ISSUES!
// It seems with specularity as if the light is coming from the opposite direction
void main()
{
    mat4 toCameraSpace = sceneData.viewMatrix * transformationMatrix;
    vec4 transformedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformedPos;
    var_texCoord = texCoord;

    vec3 transformedNormal = normalize(toCameraSpace * vec4(normal, 0.0)).xyz;

    vec3 transformedTangent = normalize((toCameraSpace * vec4(tangent.xyz, 0.0)).xyz);
    // T = normalize(T - dot(T, N) * N);
    transformedTangent = normalize(transformedTangent - dot(transformedTangent, transformedNormal) * transformedNormal);

    vec3 biTangent = normalize(cross(transformedNormal, transformedTangent));

    mat3 toTangentSpace = mat3(transformedTangent, biTangent, transformedNormal);
    toTangentSpace = transpose(toTangentSpace);

    vec3 toCam = sceneData.cameraPosition.xyz - transformedPos.xyz;
    var_toCamera = normalize(toTangentSpace * (sceneData.viewMatrix * vec4(toCam, 0.0)).xyz);

    var_lightDir = toTangentSpace * (sceneData.viewMatrix * vec4(sceneData.lightDirection.xyz, 0.0)).xyz;

    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;
}
