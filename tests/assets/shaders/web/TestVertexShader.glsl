precision mediump float;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;
attribute mat4 transformationMatrix;

struct Constants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform Constants constants;

varying vec3 var_normal;
varying vec2 var_texCoord;

void main() {
    vec4 translatedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = constants.projectionMatrix * constants.viewMatrix * translatedPos;
    var_normal = normal;
    var_texCoord = texCoord;
}
