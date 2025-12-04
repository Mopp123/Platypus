{{ VERSION }}

{{ VERTEX_INPUT }}

{{ PUSH_CONSTANTS }}

{{ DESCRIPTOR_SETS }}

{{ VERTEX_OUTPUT }}

void main()
{
    /*
    pos can be calculated


    */
    STS.calcVertexPosition();

    gl_Position = ;

    vec4 rotatedNormal = transformationMatrix * vec4(normal, 0.0);
    var_normal = rotatedNormal.xyz;
    var_texCoord = texCoord;
    var_fragPos = translatedPos.xyz;
    var_cameraPos = sceneData.cameraPosition.xyz;

    var_lightDir = sceneData.lightDirection.xyz;
    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;
}
