#version 330 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D texture1;
uniform mat4 uSamplerMultiplier = mat4(1.0);

void main() {
    FragColor = uSamplerMultiplier * texture(texture1, TexCoords);
}
