#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 weights;
layout(location = 2) in vec4 jointIDs;

layout(set = 0, binding = 0) uniform SceneData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
    vec4 ambientLightColor;
} sceneData;


const int maxJoints = 50;
layout(set = 1, binding = 0) uniform JointData
{
    mat4 data[maxJoints];
} jointData;

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
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * translatedPos;
}
