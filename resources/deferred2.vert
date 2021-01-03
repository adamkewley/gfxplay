#version 330 core

// deferred2 (vertex shader): passthrough texture shader
//
// this does nothing fancy, because it's responsible for just rendering a
// quad that fills the screen.

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
