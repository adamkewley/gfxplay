#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

uniform sampler2D uDiffuseTex;
uniform sampler2D uSpecularTex;

void main() {
    // store the fragment position vector in the first gbuffer texture
    gPosition = fs_in.FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(fs_in.Normal);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(uDiffuseTex, fs_in.TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(uSpecularTex, fs_in.TexCoords).r;
}
