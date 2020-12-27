#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fsi;

uniform sampler2D uDiffuseTex;
uniform vec3 uLightPositions[16];
uniform vec3 uLightColors[16];

out vec4 outFragColor;

void main() {
    vec3 color = texture(uDiffuseTex, fsi.TexCoords).rgb;
    vec3 normal = normalize(fsi.Normal);

    // ambient
    vec3 ambient = 0.0f * color;

    // lighting
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < 16; i++) {
        vec3 lightPos = uLightPositions[i];
        vec3 lightColor = uLightColors[i];

        // diffuse
        vec3 frag2lightDir = normalize(lightPos - fsi.FragPos);
        float diffuse_str = max(dot(frag2lightDir, normal), 0.0f);
        vec3 diffuse = lightColor * diffuse_str * color;

        vec3 result = diffuse;

        // attenuation (use quadratic as we have gamma correction)
        float distance = length(lightPos - fsi.FragPos);
        float attenuation = 1.0f / (distance * distance);
        result *= attenuation;

        lighting += result;
    }

    outFragColor = vec4(ambient + lighting, 1.0);
}

