precision mediump float;

attribute vec3 position;
attribute mat4 transformationMatrix;

struct SceneData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform SceneData sceneData;


void main() {
    vec4 translatedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * translatedPos;
}
