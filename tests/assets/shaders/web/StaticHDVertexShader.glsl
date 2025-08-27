precision mediump float;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;
attribute vec4 tangent;
attribute mat4 transformationMatrix;

struct SceneData
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColor;
    vec4 ambientLightColor;
};
uniform SceneData sceneData;

varying vec2 var_texCoord;
varying vec3 var_toCamera; // in tangent space
varying vec3 var_lightDir; // in tangent space
varying vec4 var_lightColor;
varying vec4 var_ambientLightColor;

mat3 transpose(mat3 matrix)
{
    mat3 result;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result[i][j] = matrix[j][i];
        }
    }
    return result;
}

// OLD BELOW -> seems solved for now..
// NOTE: ISSUES!
// It seems with specularity as if the light is coming from the opposite direction
void main() {
    mat4 toCameraSpace = sceneData.viewMatrix * transformationMatrix;
    vec4 transformedPos = transformationMatrix * vec4(position, 1.0);
    gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * transformedPos;
    var_texCoord = texCoord;

    vec3 transformedNormal = normalize(toCameraSpace * vec4(normal, 0.0)).xyz;

    vec3 transformedTangent = normalize((toCameraSpace * vec4(tangent.xyz, 0.0)).xyz);
    // T = normalize(T - dot(T, N) * N);
    transformedTangent = normalize(transformedTangent - dot(transformedTangent, transformedNormal) * transformedNormal);

    vec3 biTangent = normalize(cross(transformedNormal, transformedTangent));

    mat3 toTangentSpace = mat3(transformedTangent, biTangent, transformedNormal);
    toTangentSpace = transpose(toTangentSpace);

    vec3 toCam = sceneData.cameraPosition.xyz - transformedPos.xyz;
    var_toCamera = normalize(toTangentSpace * (sceneData.viewMatrix * vec4(toCam, 0.0)).xyz);

    var_lightDir = toTangentSpace * (sceneData.viewMatrix * vec4(sceneData.lightDirection.xyz, 0.0)).xyz;

    var_lightColor = sceneData.lightColor;
    var_ambientLightColor = sceneData.ambientLightColor;
}
