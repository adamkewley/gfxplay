#version 330 core

in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main(void) {
    // light distance in worldspace
    float lightDistance = length(FragPos.xyz - lightPos);

    // normalize to [0, 1]
    lightDistance = lightDistance / far_plane;

    // this shader just populates a depthmap, which is used for shadow probing
    // in a second pass
    gl_FragDepth = lightDistance;
}
