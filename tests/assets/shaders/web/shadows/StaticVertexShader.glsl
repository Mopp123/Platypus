precision mediump float;

attribute vec3 position;
attribute mat4 transformationMatrix;

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

void main()
{
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformationMatrix * vec4(position, 1.0);
}
