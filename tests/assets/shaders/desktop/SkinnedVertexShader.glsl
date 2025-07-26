#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 3) in vec4 weights;
layout(location = 4) in vec4 jointIDs;


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


const int maxJoints = 50;
layout(set = 2, binding = 0) uniform JointData
{
    mat4 data[maxJoints];
} jointData;

layout(set = 2, binding = 1) uniform InverseBindMatrices
{
    mat4 data[maxJoints];
} inverseBindMatrices;

layout(location = 0) out vec3 var_normal;
layout(location = 1) out vec2 var_texCoord;
layout(location = 2) out vec3 var_fragPos;
layout(location = 3) out vec3 var_cameraPos;
layout(location = 4) out vec3 var_lightDir;
layout(location = 5) out vec4 var_lightColor;

void main() {
    //vec4 translatedPos = constants.transformationMatrix * vec4(position, 1.0);
    //gl_Position = constants.projectionMatrix * camera.viewMatrix * translatedPos;
    //vec4 rotatedNormal = constants.transformationMatrix * vec4(normal, 0.0);


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

    vec4 translatedPos = jointTransform * vec4(position, 1.0);
    gl_Position = constants.projectionMatrix * camera.viewMatrix * translatedPos;
    vec4 rotatedNormal = vec4(normal, 0.0);

    var_normal = rotatedNormal.xyz;
    var_texCoord = texCoord;
    var_fragPos = translatedPos.xyz;
    var_cameraPos = camera.position.xyz;

    var_lightDir = directionalLight.direction.xyz;
    var_lightColor = directionalLight.color;
}
