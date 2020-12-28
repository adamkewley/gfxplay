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

uniform vec3 uLightColor;

void main() {
    FragColor = vec4(uLightColor, 1.0);

    // multiple render targets (MRTs) thresholding
    //
    // if output brightness exceeds threshold, write it to the "brightcolor"
    // output. Using MRTs means that we can do this in one fragment shader run,
    // rather than having to have a separate thresholding shader.
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(FragColor.rgb, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
