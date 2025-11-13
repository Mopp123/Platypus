#version 300 es
precision mediump float;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 weights;
layout(location = 2) in vec4 jointIDs;

struct PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform PushConstants pushConstants;

const int maxJoints = 50;
layout(std140) uniform JointData
{
    mat4 data[maxJoints];
} jointData;

void main()
{
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
    gl_Position = pushConstants.projectionMatrix * pushConstants.viewMatrix * translatedPos;
}
