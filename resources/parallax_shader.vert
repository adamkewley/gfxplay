#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec2 TexCoords;
out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normalMatrix;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vec3 FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;

    vec3 T = normalize(vec3(normalMatrix * vec4(aTangent,   0.0)));
    vec3 B = normalize(vec3(normalMatrix * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(normalMatrix * vec4(aNormal,    0.0)));
    // mat3(TBN) is a tangent-to-world matrix. Because it's an orthogonal matrix,
    // Transposing it is equivalent to taking its inverse so this TBN here is
    // a world-to-tangent transform
    mat3 TBN = transpose(mat3(T, B, N));

    TangentLightPos = TBN * lightPos;
    TangentViewPos = TBN * viewPos;
    TangentFragPos = TBN * FragPos;
}
