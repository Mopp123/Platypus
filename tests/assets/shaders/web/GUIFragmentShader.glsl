precision mediump float;

varying vec2 var_texCoord;

uniform sampler2D textureSampler;

void main() {
    vec4 textureColor = texture2D(textureSampler, var_texCoord);
    gl_FragColor = textureColor;
}
