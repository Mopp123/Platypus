#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 transform;
layout(location = 2) in vec2 textureOffset;
layout(location = 3) in vec4 color;

layout(push_constant) uniform Constants
{
    mat4 projectionMatrix;
    float textureAtlasRows;
} constants;

layout(location = 0) out vec2 var_texCoord;
layout(location = 1) out vec4 var_color;

void main() {
    vec2 scaledVertex = position * transform.zw;
    // NOTE: orthographic proj mat and viewport setup the way that we need to negate on y to have
    // 0,0 be top left corner
    // TODO: Some better solution for this
    vec2 positionedVertex = vec2(scaledVertex.x + transform.x, scaledVertex.y - transform.y);
    gl_Position = constants.projectionMatrix * vec4(positionedVertex, 0, 1.0);
    var_texCoord = vec2(position.x + textureOffset.x, -position.y + textureOffset.y) / constants.textureAtlasRows;
    var_color = color;
}
