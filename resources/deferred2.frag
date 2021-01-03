#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
struct Light {
    vec3 Position;
    vec3 Color;

    float Linear;
    float Quadratic;
    float Radius;
};
const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

void main() {
    // deferred shading: sample scene variables from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    // calculate Blinn-Phong lighting as usual
    vec3 ambient = 0.05 * Diffuse;
    vec3 frag2ViewDir = normalize(viewPos - FragPos);

    vec3 lighting = ambient;
    for (int i = 0; i < NR_LIGHTS; ++i) {
        // calculate distance between light source and current fragment
        float distance = length(lights[i].Position - FragPos);

        // light volumes: only calculate lighting if the frag is within some
        //                radius of the light (perf optimization)
        if (distance < lights[i].Radius) {
            vec3 frag2LightDir = normalize(lights[i].Position - FragPos);

            // diffuse
            float diff = max(dot(Normal, frag2LightDir), 0.0);
            vec3 diffuse = diff * Diffuse * lights[i].Color;

            // specular
            vec3 halfwayDir = normalize(frag2LightDir + frag2ViewDir);
            float spec = pow(max(dot(Normal, halfwayDir), 0.0), 32.0);
            vec3 specular = spec * Specular * lights[i].Color;

            // attenuation
            float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
    }

    // also perform gamma correction
    const float gamma = 2.2;
    lighting = pow(lighting, vec3(1.0 / gamma));

    FragColor = vec4(lighting, 1.0);
}
