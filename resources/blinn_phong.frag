#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool blinn;

void main() {
    vec3 color = texture(texture1, TexCoords).rgb;
    // ambient
    vec3 ambient = 0.00000005 * color;
    // diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 normal = normalize(Normal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;

    if (blinn) {
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    } else {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    }

    vec3 specular = vec3(0.3) * spec; // assuming bright white light color

    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
