#version 330 core

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D normalMap;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    vec3 color = texture(texture1, TexCoords).rgb;
    vec3 normal = texture(normalMap, TexCoords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(TBN * normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 lightColor = vec3(5.0);

    // ambient
    vec3 ambient = 0.1 * lightColor;

    // diffuse
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);

    vec3 specular = spec * lightColor; // assuming bright white light color

    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (distance * distance);
    //diffuse *= attenuation;
    //specular *= attenuation;

    FragColor = vec4((ambient + diffuse + specular) * color, 1.0);
}
