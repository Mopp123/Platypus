precision mediump float;

attribute vec3 position;
attribute vec4 weights;
attribute vec4 jointIDs;

struct PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform PushConstants pushConstants;

const int maxJoints = 50;
uniform mat4 jointData[maxJoints];

void main() {
    float weightSum = weights[0] + weights[1] + weights[2] + weights[3];
    mat4 jointTransform = jointData[0];
    if (weightSum >= 1.0)
    {
        jointTransform =  jointData[int(jointIDs[0])] * weights[0];
	    jointTransform += jointData[int(jointIDs[1])] * weights[1];
	    jointTransform += jointData[int(jointIDs[2])] * weights[2];
	    jointTransform += jointData[int(jointIDs[3])] * weights[3];
    }
    else
    {
        jointTransform = jointData[int(jointIDs[0])];
    }

    vec4 translatedPos = jointTransform * vec4(position, 1.0);
    gl_Position = pushConstants.projectionMatrix * pushConstants.viewMatrix * translatedPos;
}
