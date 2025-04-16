precision mediump float;

attribute vec2 position;
attribute vec2 texCoord;
attribute mat4 transformationMatrix;

struct Constants
{
    mat4 projectionMatrix;
};
uniform Constants constants;

varying vec2 var_texCoord;

void main() {
    gl_Position = constants.projectionMatrix * transformationMatrix * vec4(position, 0.0, 1.0);
    var_texCoord = texCoord;
}
