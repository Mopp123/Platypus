precision mediump float;

varying vec3 var_normal;
varying vec2 var_texCoord;

uniform sampler2D textureSampler;


void main() {
    gl_FragColor = texture2D(textureSampler, var_texCoord) + (vec4(var_normal, 1.0) * 0.01);
}
