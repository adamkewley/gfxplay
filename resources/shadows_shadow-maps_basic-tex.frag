#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D tex;

void main(void) {
    float depthValue = texture(tex, TexCoord).r;
    FragColor = vec4(vec3(depthValue), 1.0);
}
