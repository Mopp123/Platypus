precision mediump float;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;
attribute mat4 transformationMatrix;

struct Constants
{
    mat4 projectionMatrix;
};
uniform Constants constants;

struct Camera
{
    vec4 position;
    mat4 viewMatrix;
};
uniform Camera camera;

varying vec3 var_normal;
varying vec2 var_texCoord;

varying vec3 var_cameraPos;

void main() {
    vec4 translatedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = constants.projectionMatrix * camera.viewMatrix * translatedPos;
    vec4 translatedNormal = transformationMatrix * vec4(normal, 0.0);
    var_normal = translatedNormal.xyz;
    var_texCoord = texCoord;

    var_cameraPos = camera.position.xyz;
}
