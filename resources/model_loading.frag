#version 330 core

out vec4 FragColor;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform DirLight light;

#define MAX_DIFFUSE_TEXTURES 4
uniform sampler2D diffuseTextures[MAX_DIFFUSE_TEXTURES];
uniform int activeDiffuseTextures;
#define MAX_SPECULAR_TEXTURES 4
uniform sampler2D specularTextures[MAX_SPECULAR_TEXTURES];
uniform int activeSpecularTextures;

void main() {
    // Phong shader

    // ambient + diffuse: texture-independent vars
    vec3 norm = normalize(Normal);
    vec3 dirTowardsLight = normalize(-light.direction);
    float diffuseStrength = max(dot(norm, dirTowardsLight), 0.0);

    // ambient:
    //     surfaces are lit, regardless of orientation
    // diffuse:
    //     surfaces pointed towards light are lit more
    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    for (int i = 0; i < activeDiffuseTextures; ++i) {
        vec3 textel = vec3(texture(diffuseTextures[i], TexCoords));

        ambient += light.ambient * textel;
        diffuse += light.diffuse * diffuseStrength * textel;
    }

    // specular: texture-independent vars
    vec3 dirAwayFromLight = -dirTowardsLight;
    vec3 lightToViewReflect = reflect(norm, dirAwayFromLight);
    vec3 fragToView = normalize(viewPos - FragPos);
    float specularScaling = max(dot(fragToView, lightToViewReflect), 0.0);
    float materialShininess = 32.0;
    float specularAmount = pow(specularScaling, materialShininess);

    // specular: light that bounces from the light into the camera creates
    //           highlights - if the material is shiny enough (decided by a
    //           specular texture)
    vec3 specular = vec3(0.0);
    for (int i = 0; i < activeSpecularTextures; ++i) {
        vec3 textel = vec3(texture(specularTextures[i], TexCoords));
        specular += light.specular * specularAmount * textel;
    }

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
