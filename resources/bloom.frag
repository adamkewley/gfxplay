#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

// MRT: this fragment shader assumes multiple render targets have been assigned
// by the caller
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];
uniform sampler2D uDiffuseTex;

void main() {
    vec3 color = texture(uDiffuseTex, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);

    // ambient
    vec3 ambient = 0.0 * color;

    // lighting
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        vec3 lightColor = uLightColors[i];
        vec3 lightPos = uLightPositions[i];

        // diffuse
        vec3 lightDir = normalize(lightPos - fs_in.FragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 result = lightColor * diff * color;
        // attenuation (use quadratic as we have gamma correction)
        float distance = length(fs_in.FragPos - lightPos);
        result *= 1.0 / (distance * distance);
        lighting += result;
    }
    vec3 result = ambient + lighting;

    // multiple render targets (MRTs) thresholding
    //
    // if output brightness exceeds threshold, write it to the "brightcolor"
    // output. Using MRTs means that we can do this in one fragment shader run,
    // rather than having to have a separate thresholding shader.

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(result, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    FragColor = vec4(result, 1.0);
}

