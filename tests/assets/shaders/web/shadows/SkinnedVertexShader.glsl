precision mediump float;

attribute vec3 position;
attribute vec4 weights;
attribute vec4 jointIDs;

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
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * translatedPos;
}
