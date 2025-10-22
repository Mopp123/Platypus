precision mediump float;

attribute vec3 position;
attribute mat4 transformationMatrix;

struct PushConstants
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
};
uniform PushConstants pushConstants;

void main()
{
    gl_Position = pushConstants.projectionMatrix * pushConstants.viewMatrix * transformationMatrix * vec4(position, 1.0);
}
