precision mediump float;

varying vec2 var_texCoord;
varying vec4 var_color;

uniform sampler2D textureSampler;

void main() {
    vec4 textureColor = texture2D(textureSampler, var_texCoord) * var_color;
    gl_FragColor = textureColor;
}
