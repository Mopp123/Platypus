precision mediump float;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;
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
// Below struct thing doesn't work here for some reason...
//struct JointData
//{
//    mat4 data[maxJoints];
//};
//uniform JointData jointData;
uniform mat4 jointData[maxJoints];

varying vec3 var_normal;
varying vec2 var_texCoord;
varying vec3 var_fragPos;
varying vec3 var_cameraPos;
varying vec3 var_lightDir;
varying vec4 var_lightColor;
varying vec4 var_ambientLightColor;

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
    vec4 rotatedNormal = vec4(normal, 0.0);

    var_normal = rotatedNormal.xyz;
    var_texCoord = texCoord;
    var_fragPos = translatedPos.xyz;
    var_cameraPos = sceneData.cameraPosition.xyz;

    var_lightDir = sceneData.lightDirection.xyz;
    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;
}
