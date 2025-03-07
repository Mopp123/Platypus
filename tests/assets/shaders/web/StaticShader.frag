precision mediump float;

varying vec3 var_normal;
varying vec2 var_uvCoord;
varying vec4 var_ambientColor;
varying vec3 var_fragPos;
varying vec3 var_camPos;

varying float var_distToCam;


struct DirectionalLight
{
    vec4 direction;
    vec4 color;
};
uniform DirectionalLight directionalLight;


struct Material
{
    sampler2D diffuseTexSampler0;
    sampler2D specularTexSampler;
    vec4 color;
    // Properties' values:
    // x = specular strength
    // y = specular shininess
    // z = shadeless(0 or 1)
    vec4 properties;
};
uniform Material material;

// For gui to properly get rendered we have to discard some fragments
// here... otherwise the 3d bleeds through gui when model's face
// "impales" the camera (clipping due to zNear)
// TODO: Some better way of dealing with this!
const float overrideZNear = 1.0;

void main()
{
    if (var_distToCam <= overrideZNear)
        discard;

    float isShadeless = material.properties.z;
    vec4 diffuseTexColor0 = texture2D(material.diffuseTexSampler0, var_uvCoord) * material.color;

    if (diffuseTexColor0.a < 0.05)
        discard;

    if (isShadeless == 0.0)
    {
        float specularStrength = material.properties.x;
        float specularShininess = material.properties.y;

        vec3 normal = normalize(var_normal);
        vec3 lightDir = normalize(directionalLight.direction.xyz);

        float diffFactor = max(dot(normal, -lightDir), 0.0);
        vec4 diffuseColor = diffFactor * directionalLight.color;


        //vec3 toCam = normalize(var_fragPos - var_camPos);
        //vec3 lightReflection = reflect(lightDir, normal);
        //float specularFactor = pow(max(dot(-toCam, lightReflection), 0.0), specularShininess);

        // That Blinn-Phong thing... BUT! I have no idea why the fuck need to *-1 the light dir
        // ..its fucked otherwise but.. it doesn't make any sense!!!
        vec3 viewDir = normalize(var_camPos - var_fragPos);
        vec3 halfWay = normalize(directionalLight.direction.xyz * -1.0 + viewDir);
        float specularFactor = pow(max(dot(normal, halfWay), 0.0), specularShininess);

        vec4 specularTexColor = texture2D(material.specularTexSampler, var_uvCoord);
        vec4 specularColor = specularStrength * specularFactor * directionalLight.color * specularTexColor;

        gl_FragColor = (var_ambientColor + diffuseColor + specularColor) * diffuseTexColor0;
    }
    else
    {
        gl_FragColor = diffuseTexColor0;
    }
}
