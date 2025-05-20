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


struct DirectionalLight
{
    vec4 direction;
    vec4 color;
};
uniform DirectionalLight directionalLight;


varying vec3 var_normal;
varying vec2 var_texCoord;
varying vec3 var_fragPos;
varying vec3 var_cameraPos;
varying vec4 var_lightDir;
varying vec4 var_lightColor;


void main() {
    vec4 translatedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = constants.projectionMatrix * camera.viewMatrix * translatedPos;
    vec4 rotatedNormal = transformationMatrix * vec4(normal, 0.0);
    var_normal = rotatedNormal.xyz;
    var_texCoord = texCoord;
    var_fragPos = translatedPos.xyz;
    var_cameraPos = camera.position.xyz;

    var_lightDir = directionalLight.direction;
    var_lightColor = directionalLight.color;
}
