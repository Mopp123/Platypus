#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 tangent;
layout(location = 4) in mat4 transformationMatrix;

layout(push_constant) uniform Constants
{
    mat4 projectionMatrix;
} constants;


layout(set = 0, binding = 0) uniform Camera
{
    vec4 position;
    mat4 viewMatrix;
} camera;


layout(set = 1, binding = 0) uniform DirectionalLight
{
    vec4 direction;
    vec4 color;
} directionalLight;


layout(location = 0) out vec3 var_normal;
layout(location = 1) out vec2 var_texCoord;
layout(location = 2) out vec3 var_fragPos; // in tangent space
layout(location = 3) out vec3 var_toCamera; // in tangent space
layout(location = 4) out vec3 var_lightDir; // in tangent space
layout(location = 5) out vec4 var_lightColor; // in tangent space
layout(location = 6) out mat3 var_toTangentSpace;
layout(location = 9) out vec4 var_tangent;


// NOTE: ISSUES!
// It seems with specularity as if the light is coming from the opposite direction
void main() {
    mat4 toCameraSpace = camera.viewMatrix * transformationMatrix;
    vec4 transformedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = constants.projectionMatrix * camera.viewMatrix * transformedPos;
    var_texCoord = texCoord;


    var_fragPos = transformedPos.xyz;
    vec3 transformedNormal = normalize(toCameraSpace * vec4(normal, 0.0)).xyz;

    vec3 transformedTangent = normalize((toCameraSpace * vec4(tangent.xyz, 0.0)).xyz);
    // T = normalize(T - dot(T, N) * N);
    transformedTangent = normalize(transformedTangent - dot(transformedTangent, transformedNormal) * transformedNormal);

    vec3 biTangent = normalize(cross(transformedNormal, transformedTangent));

    var_toTangentSpace = transpose(mat3(transformedTangent, biTangent, transformedNormal));

    vec3 toCam = camera.position.xyz - transformedPos.xyz;
    var_toCamera = normalize(var_toTangentSpace * (camera.viewMatrix * vec4(toCam, 0.0)).xyz);

    var_lightDir = var_toTangentSpace * (camera.viewMatrix * vec4(directionalLight.direction.xyz, 0.0)).xyz;

    var_lightColor = directionalLight.color;
    var_normal = (toCameraSpace * vec4(normal, 0.0)).xyz;

    var_tangent = vec4(biTangent, 1.0);
}
