precision mediump float;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;

struct Constants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform Constants constants;

struct Test
{
    mat4 transformationMatrix;
};
uniform Test test;

varying vec3 var_normal;
varying vec2 var_texCoord;

void main() {
    vec4 translatedPos = test.transformationMatrix * vec4(position, 1.0);
    gl_Position = constants.projectionMatrix * constants.viewMatrix * translatedPos;
    var_normal = normal;
    var_texCoord = texCoord;
}
