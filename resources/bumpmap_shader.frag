#version 330 core

in vec2 TexCoords;
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D normalMap;

void main() {
    vec3 color = texture(texture1, TexCoords).rgb;

    // normal map is in tangent space and in the range [0,1]
    // remap to [-1,1] (so we can go "down" as well as "up")
    vec3 normal = texture(normalMap, TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    // lighting calculations are performed in tangent space

    vec3 tangentLightDir = normalize(TangentLightPos - TangentFragPos);
    vec3 lightColor = vec3(5.0);

    // ambient
    vec3 ambient = 0.1 * lightColor;

    // diffuse
    float diff = max(dot(tangentLightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular (Blinn-Phong)
    vec3 tangentViewDir = normalize(TangentViewPos - TangentFragPos);
    vec3 tangentReflectDir = reflect(-tangentLightDir, normal);
    vec3 tangentHalfwayDir = normalize(tangentLightDir + tangentViewDir);
    float spec = pow(max(dot(normal, tangentHalfwayDir), 0.0), 64.0);

    vec3 specular = spec * lightColor; // assuming bright white light color

    float distance = length(TangentLightPos - TangentFragPos);
    float attenuation = 1.0 / (distance * distance);
    //diffuse *= attenuation;
    //specular *= attenuation;

    FragColor = vec4((ambient + diffuse + specular) * color, 1.0);
}
