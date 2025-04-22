precision mediump float;

varying vec2 var_texCoord;
varying vec4 var_color;

uniform sampler2D textureSampler;

void main() {
    vec4 textureColor = texture2D(textureSampler, var_texCoord);
    float intensity = textureColor.a;

    vec4 totalColor = vec4(var_color.rgb, intensity);
    gl_FragColor = totalColor;
}
