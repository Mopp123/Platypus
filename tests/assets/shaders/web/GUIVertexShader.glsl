precision mediump float;

attribute vec2 position;
attribute vec4 transform;
attribute vec2 textureOffset;

struct Constants
{
    mat4 projectionMatrix;
    float textureAtlasRows;
};
uniform Constants constants;

varying vec2 var_texCoord;

void main() {
    vec2 scaledVertex = position * transform.zw;
    // NOTE: orthographic proj mat and viewport setup the way that we need to negate on y to have
    // 0,0 be top left corner
    // TODO: Some better solution for this
    vec2 positionedVertex = vec2(scaledVertex.x + transform.x, scaledVertex.y - transform.y);
    gl_Position = constants.projectionMatrix * vec4(positionedVertex, 0, 1.0);
    var_texCoord = vec2(position.x + textureOffset.x, -position.y + textureOffset.y) / constants.textureAtlasRows;
}
