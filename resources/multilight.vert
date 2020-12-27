#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 uModelMtx;
uniform mat4 uViewMtx;
uniform mat4 uProjMtx;
uniform mat3 uNormalMtx;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vso;

out vec3 FragPos;
out vec3 Normal;
out vec2 outTexCoords;

void main() {
    vso.FragPos = vec3(uModelMtx * vec4(aPos, 1.0f));
    vso.Normal = uNormalMtx * aNormal;
    vso.TexCoords = aTexCoords;

    gl_Position = uProjMtx * uViewMtx * uModelMtx * vec4(aPos, 1.0f);
}
